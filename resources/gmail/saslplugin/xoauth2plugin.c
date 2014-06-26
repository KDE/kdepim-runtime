/*
    Copyright (c) 2014 Daniel Vrátil <dvratil@redhat.com>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

/**
 * This is a very simple and single-purpose implementation of XOAUTH2 protocol
 * for SASL used by KDE's KIMAP and Gmail resource in order to support OAuth
 * Gmail authentication.
 *
 * This plugin does not even have a server-side part, and the client-side makes
 * assumptions about it's use related to how KIMAP's LoginJob is implemented,
 * so it's unsuitable for general-purpose use.
 *
 * The plugin is called libkdexoauth2.so to avoid conflict in case anyone ever
 * writes a proper XOAUTH2 plugin for SASL.
 */

#include <stdio.h>
#include <string.h>
#include <sasl/sasl.h>
#include <sasl/saslplug.h>
#include <sasl/saslutil.h>

#include "plugin_common.h"

/***************************** Common Section *****************************/

static const char plugin_id[] = "$Id: plain.c,v 1.64 2004/09/08 11:06:11 mel Exp $";


/***************************** Client Section *****************************/

typedef struct client_context {
    char *out_buf;
    unsigned out_buf_len;
} client_context_t;

static int xoauth2_client_mech_new(void *glob_context,
                                   sasl_client_params_t *params,
                                   void **conn_context)
{
    client_context_t *context;

    /* Q_UNUSED */
    (void)glob_context;

    /* holds state are in */
    context = params->utils->malloc(sizeof(client_context_t));
    if (context == NULL) {
        MEMERROR(params->utils);
        return SASL_NOMEM;
    }

    memset(context, 0, sizeof(client_context_t));

    *conn_context = context;

    return SASL_OK;
}

static int xoauth2_client_mech_step(void *conn_context,
                                    sasl_client_params_t *params,
                                    const char *serverin,
                                    unsigned serverinlen,
                                    sasl_interact_t **prompt_need,
                                    const char **clientout,
                                    unsigned *clientoutlen,
                                    sasl_out_params_t *oparams)
{
    client_context_t *context = (client_context_t *) conn_context;
    const sasl_utils_t *utils = params->utils;
    const char *authid = NULL, *token = NULL;
    int auth_result = SASL_OK;
    int token_result = SASL_OK;
    int result;
    *clientout = NULL;
    *clientoutlen = 0;

    /* Q_UNUSED */
    (void) serverin;
    (void) serverinlen;

    if (params->props.min_ssf > params->external_ssf) {
        SETERROR(params->utils, "SSF requested of XOAUTH2 plugin");
        return SASL_TOOWEAK;
    }

    /* try to get the authid */
    if (oparams->authid == NULL) {
        auth_result = _plug_get_authid(utils, &authid, prompt_need);
        if ((auth_result != SASL_OK) && (auth_result != SASL_INTERACT)) {
            return auth_result;
        }
    }

    /* try to get oauth token */
    if (token == NULL) {
        /* We don't use _plug_get_password because we don't really care much about
           safety of the OAuth token */
        token_result = _plug_get_simple(utils, SASL_CB_PASS, 1, &token, prompt_need);
        if ((token_result != SASL_OK) && (token_result != SASL_INTERACT)) {
            return token_result;
        }
    }

    /* free prompts we got */
    if (prompt_need && *prompt_need) {
        utils->free(*prompt_need);
        *prompt_need = NULL;
    }

    /* if there are prompts not filled in */
    if ((auth_result == SASL_INTERACT) ||(token_result == SASL_INTERACT)) {
        /* make the prompt list */
        result =
            _plug_make_prompts(utils, prompt_need,
                               NULL, NULL,
                               auth_result == SASL_INTERACT ?
                               "Please enter your authentication name" : NULL,
                               NULL,
                               token_result == SASL_INTERACT ?
                               "Please enter OAuth token" : NULL, NULL,
                               NULL, NULL, NULL,
                               NULL, NULL, NULL);
        if (result != SASL_OK) {
            return result;
        }

        return SASL_INTERACT;
    }

    /* FIXME: Fail if username is missing too? */
    if (!token) {
        PARAMERROR(params->utils);
        return SASL_BADPARAM;
    }

    result = params->canon_user(utils->conn, authid, 0,
                                SASL_CU_AUTHID | SASL_CU_AUTHZID, oparams);
    if (result != SASL_OK) {
        return result;
    }

    /* https://developers.google.com/gmail/xoauth2_protocol#the_sasl_xoauth2_mechanism */
    *clientoutlen = 5                                               /* user=*/
                  + ((authid && *authid) ? strlen(authid) : 0)      /* %s */
                  + 1                                               /* \001 */
                  + 12                                              /* auth=Bearer{space} */
                  + ((token && *token) ? strlen(token) : 0)         /* %s */
                  + 2;                                              /* \001\001 */

    /* remember the extra NUL on the end for stupid clients */
    result = _plug_buf_alloc(params->utils, &(context->out_buf),
                             &(context->out_buf_len), *clientoutlen + 1);
    if (result != SASL_OK) {
        return result;
    }

    snprintf(context->out_buf, context->out_buf_len, "user=%s\1auth=Bearer %s\1\1", authid, token);

    *clientout = context->out_buf;

    /* set oparams */
    oparams->doneflag = 1;
    oparams->mech_ssf = 0;
    oparams->maxoutbuf = 0;
    oparams->encode_context = NULL;
    oparams->encode = NULL;
    oparams->decode_context = NULL;
    oparams->decode = NULL;
    oparams->param_version = 0;

    return SASL_OK;
}

static void xoauth2_client_mech_dispose(void *conn_context,
                                        const sasl_utils_t *utils)
{
    client_context_t *context = (client_context_t *) conn_context;
    if (!context) {
        return;
    }

    if (context->out_buf) {
        printf("%s", context->out_buf);
        utils->free(context->out_buf);
    }
    utils->free(context);
}

static sasl_client_plug_t xoauth2_client_plugins[] =
{
    {
        "XOAUTH2",                      /* mech_name */
        0,                              /* max_ssf */
        SASL_SEC_NOANONYMOUS
        | SASL_SEC_PASS_CREDENTIALS,    /* security_flags */
        SASL_FEAT_WANT_CLIENT_FIRST
        | SASL_FEAT_ALLOWS_PROXY,       /* features */
        NULL,                           /* required_prompts */
        NULL,                           /* glob_context */
        &xoauth2_client_mech_new,       /* mech_new */
        &xoauth2_client_mech_step,      /* mech_step */
        &xoauth2_client_mech_dispose,   /* mech_dispose */
        NULL,                           /* mech_free */
        NULL,                           /* idle */
        NULL,                           /* spare */
        NULL                            /* spare */
    }
};

int xoauth2_client_plug_init(sasl_utils_t *utils,
                             int maxversion,
                             int *out_version,
                             sasl_client_plug_t **pluglist,
                             int *plugcount)
{
    if (maxversion < SASL_CLIENT_PLUG_VERSION) {
        SETERROR(utils, "XOAUTH2 version mismatch");
        return SASL_BADVERS;
    }

    *out_version = SASL_CLIENT_PLUG_VERSION;
    *pluglist = xoauth2_client_plugins;
    *plugcount = 1;

    return SASL_OK;
}
