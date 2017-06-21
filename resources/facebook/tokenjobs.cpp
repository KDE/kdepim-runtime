/*
 *    Copyright (C) 2017 Daniel Vrátil <dvratil@kde.org>
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "tokenjobs.h"
#include "resource.h"
#include "resource_debug.h"
#include "graph.h"

#include <QDialog>
#include <QVBoxLayout>
#include <QGlobalStatic>
#include <QTimer>
#include <QContextMenuEvent>
#include <QToolButton>
#include <QMessageBox>
#include <QLineEdit>
#include <QProgressBar>

#include <QJsonDocument>
#include <QJsonObject>

#include <QWebEngineView>
#include <QWebEngineSettings>
#include <QWebEnginePage>
#include <QWebEngineProfile>
#include <QWebEngineCookieStore>

#include <KWallet/Wallet>

#include <KLocalizedString>

namespace {

static const auto KWalletFolder = QStringLiteral("Facebook");
static const auto KWalletKeyToken = QStringLiteral("token");
static const auto KWalletKeyName = QStringLiteral("name");
static const auto KWalletKeyId = QStringLiteral("id");
static const auto KWalletKeyCookies = QStringLiteral("cookies");

class TokenManager
{
public:
    ~TokenManager()
    {
        delete wallet;
        wallet = nullptr;
    }

    KWallet::Wallet *wallet = nullptr;
    QString token;
    QString userName;
    QString id;
    QByteArray cookies;
};

Q_GLOBAL_STATIC(TokenManager, d)

class WebView : public QWebEngineView
{
    Q_OBJECT
public:
    explicit WebView(QWidget *parent = nullptr)
        : QWebEngineView(parent)
    {
        QWebEngineProfile::defaultProfile()->setPersistentCookiesPolicy(QWebEngineProfile::NoPersistentCookies);
    }

    void contextMenuEvent(QContextMenuEvent *e) override
    {
        e->accept();
    }
};

class WebPage : public QWebEnginePage
{
    Q_OBJECT
public:
    explicit WebPage(QWebEngineProfile *profile, QObject *parent = nullptr)
        : QWebEnginePage(profile, parent)
    {}

    QWebEngineCertificateError *lastCeritificateError() const
    {
        return mLastError;
    }

    bool certificateError(const QWebEngineCertificateError &err) override
    {
        if (mLastError) {
            delete mLastError;
        }
        mLastError = new QWebEngineCertificateError(err.error(), err.url(), err.isOverridable(), err.errorDescription());
        Q_EMIT sslError({});

        return false;
    }

Q_SIGNALS:
    void sslError(QPrivateSignal);

private:
    QWebEngineCertificateError *mLastError = nullptr;
};

class AuthDialog : public QDialog
{
    Q_OBJECT

public:
    AuthDialog(const QByteArray &cookies, FacebookResource *resource,
               QWidget *parent = nullptr)
        : QDialog(parent)
    {
        setModal(true);
        setMinimumSize(1080, 880); // minimal size to fit in facebook login screen
                                   // without scrollbars

        auto v = new QVBoxLayout(this);

        auto h = new QHBoxLayout(this);
        h->setSpacing(0);
        mSslIndicator = new QToolButton(this);
        connect(mSslIndicator, &QToolButton::clicked,
                this, [this]() {
                    auto page = qobject_cast<WebPage*>(mView->page());
                    if (auto err = page->lastCeritificateError()) {
                        QMessageBox msg;
                        msg.setIconPixmap(QIcon::fromTheme(QStringLiteral("security-low")).pixmap(64));
                        msg.setText(err->errorDescription());
                        msg.addButton(QMessageBox::Ok);
                        msg.exec();
                    }
                });
        h->addWidget(mSslIndicator);

        mUrlEdit = new QLineEdit(this);
        mUrlEdit->setReadOnly(true);
        h->addWidget(mUrlEdit);

        v->addLayout(h);

        auto progressBar = new QProgressBar(this);
        progressBar->setMinimum(0);
        progressBar->setMaximum(100);
        progressBar->setValue(0);
        v->addWidget(progressBar);

        // Create a special profile just for us
        auto profile = new QWebEngineProfile(resource->identifier(), this);
        auto cookieStore = profile->cookieStore();
        cookieStore->deleteAllCookies(); // delete all cookies from it
        const auto parsedCookies = QNetworkCookie::parseCookies(cookies);
        for (const auto &parsedCookie : parsedCookies) {
            cookieStore->setCookie(parsedCookie, QUrl(QStringLiteral(".facebook.com")));
            mCookies.insert(parsedCookie.name(), parsedCookie.toRawForm());
        }
        connect(cookieStore, &QWebEngineCookieStore::cookieAdded,
                this, [this](const QNetworkCookie &cookie) {
                    if (cookie.domain() == QLatin1String(".facebook.com")) {
                        mCookies.insert(cookie.name(), cookie.toRawForm());
                    }
                });
        connect(cookieStore, &QWebEngineCookieStore::cookieRemoved,
                this, [this](const QNetworkCookie &cookie) {
                    mCookies.remove(cookie.name());
                });

        mView = new WebView(this);
        auto webpage = new WebPage(profile, mView);
        connect(webpage, &WebPage::sslError,
                this, [this]() {
                    setSslIcon(QStringLiteral("security-low"));
                });
        mView->setPage(webpage);
        v->addWidget(mView);

        connect(mView, &WebView::loadProgress, progressBar, &QProgressBar::setValue);
        connect(mView, &WebView::urlChanged, this, &AuthDialog::onUrlChanged);

        mShowTimer = new QTimer(this);
        mShowTimer->setSingleShot(true);
        mShowTimer->setInterval(1000);
        connect(mShowTimer, &QTimer::timeout, this, &QWidget::show);
    }

    ~AuthDialog()
    {
    }

    void run()
    {
        QUrl url(QStringLiteral("https://www.facebook.com/v2.9/dialog/oauth"));
        QUrlQuery query;
        query.addQueryItem(QStringLiteral("client_id"), Graph::appId());
        query.addQueryItem(QStringLiteral("redirect_uri"), QStringLiteral("https://www.facebook.com/connect/login_success.html"));
        query.addQueryItem(QStringLiteral("response_type"), QStringLiteral("token"));
        query.addQueryItem(QStringLiteral("scope"), Graph::scopes());
        url.setQuery(query);

        mView->load(url);

        // Don't show the dialog here, we will only show it if we are stuck on
        // login.php for longer than a second
    }

    QString token() const
    {
        return mToken;
    }

    QByteArray cookies() const
    {
        QByteArray rv;
        for (auto it = mCookies.cbegin(), end = mCookies.cend(); it != end; ++it) {
            rv += it.value() + '\n';
        }
        return rv;
    }

Q_SIGNALS:
    void authDone();

private Q_SLOTS:
    void onUrlChanged(const QUrl &newUrl)
    {
        mUrlEdit->setText(newUrl.toDisplayString(QUrl::PrettyDecoded));
        mUrlEdit->setCursorPosition(0);

        if (!newUrl.host().contains(QLatin1String(".facebook.com"))) {
            setSslIcon(QStringLiteral("security-medium"));
            return;
        }

        if (qobject_cast<WebPage*>(mView->page())->lastCeritificateError()) {
            setSslIcon(QStringLiteral("security-low"));
        } else {
            setSslIcon(QStringLiteral("security-high"));
        }

        if (newUrl.path() == QLatin1String("/login.php")) {
            if (!isVisible() && !mShowTimer->isActive()) {
                // If we get stuck on login.php for at least a second, then it means
                // facebook wants user login, otherwise we are just immediatelly redirected
                // to login_success.html, which causes the webview to just flash on the
                // screen, which is not nice, and can be confusing if it happens randomly
                // when the resource is syncing in the background
                mShowTimer->start();
            }
        } else if (newUrl.path() == QLatin1String("/connect/login_success.html")) {
            mShowTimer->stop();
            QUrlQuery query(newUrl.fragment());
            mToken = query.queryItemValue(QStringLiteral("access_token"));
            hide();

            Q_EMIT authDone();
        }
    }

    void setSslIcon(const QString &iconName)
    {
        // FIXME: workaround for silly Breeze icons: the small 22x22 icons are
        // monochromatic, which is absolutely useless since we are trying to security
        // information here, so instead we force use the bigger 48x48 icons which
        // have colors and downscale them
        mSslIndicator->setIcon(QIcon::fromTheme(iconName).pixmap(48));
    }

private:
    QWebEngineView *mView = nullptr;
    QTimer *mShowTimer = nullptr;
    QToolButton *mSslIndicator = nullptr;
    QLineEdit *mUrlEdit = nullptr;
    QString mToken;
    QMap<QByteArray, QByteArray> mCookies;
};

} // namespace



TokenJob::TokenJob(FacebookResource *parent)
    : KJob(parent)
{}

TokenJob::~TokenJob()
{
}

void TokenJob::start()
{
    if (!d->wallet) {
        d->wallet = KWallet::Wallet::openWallet(KWallet::Wallet::NetworkWallet(), 0,
                                                KWallet::Wallet::Asynchronous);
        if (!d->wallet) {
            emitError(i18n("Failed to open KWallet"));
            return;
        }
    }

    if (d->wallet->isOpen()) {
        doStart();
    } else {
        connect(d->wallet, &KWallet::Wallet::walletOpened,
                this, [this]() {
                        if (!d->wallet->isOpen()) {
                            delete d->wallet;
                            d->wallet = nullptr;
                            emitError(i18n("Failed to open KWallet"));
                            return;
                        }

                        if (!d->wallet->hasFolder(KWalletFolder)) {
                            d->wallet->createFolder(KWalletFolder);
                        }

                        d->wallet->setFolder(KWalletFolder);

                        doStart();
                });
    }
}

void TokenJob::emitError(const QString &text)
{
    setError(KJob::UserDefinedError);
    setErrorText(text);
    emitResult();
}


LoginJob::LoginJob(FacebookResource *parent)
    : TokenJob(parent)
{
}

LoginJob::~LoginJob()
{
}

QString LoginJob::token() const
{
    return d->token;
}

void LoginJob::doStart()
{
    auto dlg = new AuthDialog(d->cookies, qobject_cast<FacebookResource*>(parent()));
    connect(dlg, &AuthDialog::authDone,
            this, [this, dlg]() {
                dlg->deleteLater();
                d->token = dlg->token();
                d->cookies = dlg->cookies();
                if (d->token.isEmpty()) {
                    emitError(i18n("Failed to obtain access token from Facebook"));
                    return;
                }

                fetchUserInfo();
            });

    dlg->run();
}

void LoginJob::fetchUserInfo()
{
    auto job = Graph::job(QStringLiteral("me"), d->token, { QStringLiteral("id"), QStringLiteral("name") });
    connect(job, &KJob::result,
            this, [this, job]() {
                if (job->error()) {
                    emitError(job->errorText());
                    return;
                }

                const auto json = QJsonDocument::fromJson(qobject_cast<KIO::StoredTransferJob*>(job)->data());
                const auto me = json.object();

                d->userName = me.value(QStringLiteral("name")).toString();
                d->id = me.value(QStringLiteral("id")).toString();
                d->wallet->writeMap(qobject_cast<FacebookResource*>(parent())->identifier(),
                                    { { KWalletKeyToken, d->token },
                                      { KWalletKeyName, d->userName },
                                      { KWalletKeyId, d->id },
                                      { KWalletKeyCookies, QString::fromUtf8(d->cookies) }
                                    });
                emitResult();
            });
    job->start();
}




LogoutJob::LogoutJob(FacebookResource *parent)
    : TokenJob(parent)
{
}

LogoutJob::~LogoutJob()
{
}

void LogoutJob::doStart()
{
    d->token.clear();
    d->userName.clear();
    d->id.clear();
    d->cookies.clear();

    if (!d->wallet->isOpen()) {
        emitError(i18n("Failed to open KWallet"));
        return;
    }

    d->wallet->removeEntry(qobject_cast<FacebookResource*>(parent())->identifier());
    emitResult();
}




GetTokenJob::GetTokenJob(FacebookResource *parent)
    : TokenJob(parent)
{
}

GetTokenJob::~GetTokenJob()
{
}

QString GetTokenJob::token() const
{
    return d->token;
}

QString GetTokenJob::userId() const
{
    return d->id;
}

QString GetTokenJob::userName() const
{
    return d->userName;
}

QByteArray GetTokenJob::cookies() const
{
    return d->cookies;
}

void GetTokenJob::start()
{
    // Already have token, so we are done
    if (!d->token.isEmpty()) {
        QTimer::singleShot(0, this, [this]() { emitResult(); });
        return;
    }

    TokenJob::start();
}

void GetTokenJob::doStart()
{
    if (!d->wallet->isOpen()) {
        emitError(i18n("Failed to opened KWallet"));
        return;
    }

    const auto key = qobject_cast<FacebookResource*>(parent())->identifier();
    QMap<QString, QString> entries;
    d->wallet->readMap(key, entries);
    d->token = entries.value(KWalletKeyToken);
    d->userName = entries.value(KWalletKeyName);
    d->id = entries.value(KWalletKeyId);
    d->cookies = entries.value(KWalletKeyCookies).toUtf8();

    emitResult();
}

#include "tokenjobs.moc"
