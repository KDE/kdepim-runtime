/*
    This file is part of libkdepim
    Copyright (c) 2002-2003 KDE Team

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/

// WARNING Don't add include guards here, they were removed on purpose

#include <kdeversion.h>
#include <kdemacros.h>

#if KDE_IS_VERSION( 3,3,90 )
/* life is great */
#else
/* workaround typo that breaks compilation with newer gcc */
#undef KDE_EXPORT
#define KDE_EXPORT
#undef KDE_NO_EXPORT
#define KDE_NO_EXPORT
#endif
