/*
    This file is part of Akonadi Contact.

    Copyright (c) 2009 Tobias Koenig <tokoe@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include "contacteditorwidget.h"

#include "addresseditwidget.h"
#include "contactmetadata_p.h"
#include "dateeditwidget.h"
#include "displaynameeditwidget.h"
#include "emaileditwidget.h"
#include "freebusyeditwidget.h"
#include "imagewidget.h"
#include "imeditwidget.h"
#include "nameeditwidget.h"
#include "phoneeditwidget.h"
#include "soundeditwidget.h"

#include <kconfig.h>
#include <kconfiggroup.h>
#include <klineedit.h>
#include <klocale.h>
#include <ktabwidget.h>
#include <kurlrequester.h>
#include <ktextedit.h>

#include <QtGui/QGroupBox>
#include <QtGui/QLabel>
#include <QtGui/QLayout>

class ContactEditorWidget::Private
{
  public:
    Private( ContactEditorWidget *parent )
      : mParent( parent )
    {
    }

    void initGui();
    void initGuiContactTab();
    void initGuiLocationTab();
    void initGuiBusinessTab();
    void initGuiPersonalTab();

    QString loadCustom( const KABC::Addressee &contact, const QString &key ) const;
    void storeCustom( KABC::Addressee &contact, const QString &key, const QString &value ) const;

    ContactEditorWidget *mParent;
    KTabWidget *mTabWidget;

    // widgets from name group
    NameEditWidget *mNameWidget;
    ImageWidget *mPhotoWidget;
    DisplayNameEditWidget *mDisplayNameWidget;
    KLineEdit *mNickNameWidget;
    SoundEditWidget *mPronunciationWidget;

    // widgets from internet group
    EmailEditWidget *mEmailWidget;
    KLineEdit *mHomepageWidget;
    KLineEdit *mBlogWidget;
    IMEditWidget *mIMWidget;

    // widgets from phones group
    PhoneEditWidget *mPhonesWidget;

    // widgets from addresses group
    AddressEditWidget *mAddressesWidget;

    // widgets from coordinates group
    QWidget *mCoordinatesWidget;

    // widgets from general group
    ImageWidget *mLogoWidget;
    KLineEdit *mOrganizationWidget;
    KLineEdit *mProfessionWidget;
    KLineEdit *mTitleWidget;
    KLineEdit *mDepartmentWidget;
    KLineEdit *mOfficeWidget;
    KLineEdit *mManagerWidget;
    KLineEdit *mAssistantWidget;

    // widgets from groupware group
    FreeBusyEditWidget *mFreeBusyWidget;

    // widgets from notes group
    KTextEdit *mNotesWidget;

    // widgets from dates group
    DateEditWidget *mBirthdateWidget;
    DateEditWidget *mAnniversaryWidget;

    // widgets from family group
    KLineEdit *mPartnerWidget;
};

void ContactEditorWidget::Private::initGui()
{
  QVBoxLayout *layout = new QVBoxLayout( mParent );
  layout->setMargin( 0 );

  mTabWidget = new KTabWidget( mParent );
  layout->addWidget( mTabWidget );

  initGuiContactTab();
  initGuiLocationTab();
  initGuiBusinessTab();
  initGuiPersonalTab();
}

void ContactEditorWidget::Private::initGuiContactTab()
{
  QWidget *widget = new QWidget;
  QVBoxLayout *layout = new QVBoxLayout( widget );

  mTabWidget->addTab( widget, i18n( "Contact" ) );

  QGroupBox *nameGroupBox = new QGroupBox( i18n( "Name" ) );
  QGroupBox *internetGroupBox = new QGroupBox( i18n( "Internet" ) );
  QGroupBox *phonesGroupBox = new QGroupBox( i18n( "Phones" ) );

  layout->addWidget( nameGroupBox );
  layout->addWidget( internetGroupBox );
  layout->addWidget( phonesGroupBox );

  QGridLayout *nameLayout = new QGridLayout( nameGroupBox );
  QGridLayout *internetLayout = new QGridLayout( internetGroupBox );
  QGridLayout *phonesLayout = new QGridLayout( phonesGroupBox );

  QLabel *label = 0;

  // setup name group box
  label = new QLabel( i18n( "Name:" ) );
  label->setAlignment( Qt::AlignRight | Qt::AlignVCenter );
  nameLayout->addWidget( label, 0, 0 );

  mNameWidget = new NameEditWidget;
  label->setBuddy( mNameWidget );
  nameLayout->addWidget( mNameWidget, 0, 1 );

  mPhotoWidget = new ImageWidget( ImageWidget::Photo );
  mPhotoWidget->setMinimumSize( QSize( 100, 140 ) );
  nameLayout->addWidget( mPhotoWidget, 0, 2, 4, 1 );

  label = new QLabel( i18n( "Display:" ) );
  label->setAlignment( Qt::AlignRight | Qt::AlignVCenter );
  nameLayout->addWidget( label, 1, 0 );

  mDisplayNameWidget = new DisplayNameEditWidget;
  label->setBuddy( mDisplayNameWidget );
  nameLayout->addWidget( mDisplayNameWidget, 1, 1 );

  label = new QLabel( i18n( "Nickname:" ) );
  label->setAlignment( Qt::AlignRight | Qt::AlignVCenter );
  nameLayout->addWidget( label, 2, 0 );

  mNickNameWidget = new KLineEdit;
  label->setBuddy( mNickNameWidget );
  nameLayout->addWidget( mNickNameWidget, 2, 1 );

  label = new QLabel( i18n( "Pronunciation:" ) );
  label->setAlignment( Qt::AlignRight | Qt::AlignVCenter );
  nameLayout->addWidget( label, 3, 0 );

  mPronunciationWidget = new SoundEditWidget;
  label->setBuddy( mPronunciationWidget );
  nameLayout->addWidget( mPronunciationWidget, 3, 1 );

  nameLayout->setRowStretch( 4, 1 );

  // setup internet group box
  label = new QLabel( i18n( "Email:" ) );
  label->setAlignment( Qt::AlignRight | Qt::AlignVCenter );
  internetLayout->addWidget( label, 0, 0 );

  mEmailWidget = new EmailEditWidget;
  label->setBuddy( mEmailWidget );
  internetLayout->addWidget( mEmailWidget, 0, 1 );

  label = new QLabel( i18n( "Homepage:" ) );
  label->setAlignment( Qt::AlignRight | Qt::AlignVCenter );
  internetLayout->addWidget( label, 1, 0 );

  mHomepageWidget = new KLineEdit;
  label->setBuddy( mHomepageWidget );
  internetLayout->addWidget( mHomepageWidget, 1, 1 );

  label = new QLabel( i18n( "Blog:" ) );
  label->setAlignment( Qt::AlignRight | Qt::AlignVCenter );
  internetLayout->addWidget( label, 2, 0 );

  mBlogWidget = new KLineEdit;
  label->setBuddy( mBlogWidget );
  internetLayout->addWidget( mBlogWidget, 2, 1 );

  label = new QLabel( i18n( "Messaging:" ) );
  label->setAlignment( Qt::AlignRight | Qt::AlignVCenter );
  internetLayout->addWidget( label, 3, 0 );

  mIMWidget = new IMEditWidget;
  label->setBuddy( mIMWidget );
  internetLayout->addWidget( mIMWidget, 3, 1 );

  internetLayout->setRowStretch( 4, 1 );

  // setup phones group box
  mPhonesWidget = new PhoneEditWidget;
  phonesLayout->addWidget( mPhonesWidget, 0, 0 );

  phonesLayout->setRowStretch( 1, 1 );
}

void ContactEditorWidget::Private::initGuiLocationTab()
{
  QWidget *widget = new QWidget;
  QVBoxLayout *layout = new QVBoxLayout( widget );

  mTabWidget->addTab( widget, i18n( "Location" ) );

  QGroupBox *addressesGroupBox = new QGroupBox( i18n( "Addresses" ) );
  QGroupBox *coordinatesGroupBox = new QGroupBox( i18n( "Coordinates" ) );

  layout->addWidget( addressesGroupBox );
  layout->addWidget( coordinatesGroupBox );

  QGridLayout *addressesLayout = new QGridLayout( addressesGroupBox );
  QGridLayout *coordinatesLayout = new QGridLayout( coordinatesGroupBox );

  // setup addresses group box
  mAddressesWidget = new AddressEditWidget( addressesGroupBox );
  mAddressesWidget->setMinimumHeight( 200 );
  addressesLayout->addWidget( mAddressesWidget, 0, 0 );
  addressesLayout->setRowStretch( 1, 1 );

  // setup coordinates group box
  mCoordinatesWidget = new QWidget;
  coordinatesLayout->addWidget( mCoordinatesWidget, 0, 0 );
  coordinatesLayout->setRowStretch( 1, 1 );
}

void ContactEditorWidget::Private::initGuiBusinessTab()
{
  QWidget *widget = new QWidget;
  QVBoxLayout *layout = new QVBoxLayout( widget );

  mTabWidget->addTab( widget, i18n( "Business" ) );

  QGroupBox *generalGroupBox = new QGroupBox( i18n( "General" ) );
  QGroupBox *groupwareGroupBox = new QGroupBox( i18n( "Groupware" ) );
  QGroupBox *notesGroupBox = new QGroupBox( i18n( "Notes" ) );

  layout->addWidget( generalGroupBox );
  layout->addWidget( groupwareGroupBox );
  layout->addWidget( notesGroupBox );

  QGridLayout *generalLayout = new QGridLayout( generalGroupBox );
  QGridLayout *groupwareLayout = new QGridLayout( groupwareGroupBox );
  QGridLayout *notesLayout = new QGridLayout( notesGroupBox );

  QLabel *label = 0;

  // setup general group box
  mLogoWidget = new ImageWidget( ImageWidget::Logo );
  generalLayout->addWidget( mLogoWidget, 0, 2, 6, 1, Qt::AlignTop );

  label = new QLabel( i18n( "Organization:" ) );
  label->setAlignment( Qt::AlignRight | Qt::AlignVCenter );
  generalLayout->addWidget( label, 0, 0 );

  mOrganizationWidget = new KLineEdit;
  label->setBuddy( mOrganizationWidget );
  generalLayout->addWidget( mOrganizationWidget, 0, 1 );

  label = new QLabel( i18n( "Profession:" ) );
  label->setAlignment( Qt::AlignRight | Qt::AlignVCenter );
  generalLayout->addWidget( label, 1, 0 );

  mProfessionWidget = new KLineEdit;
  label->setBuddy( mProfessionWidget );
  generalLayout->addWidget( mProfessionWidget, 1, 1 );

  label = new QLabel( i18n( "Title:" ) );
  label->setAlignment( Qt::AlignRight | Qt::AlignVCenter );
  generalLayout->addWidget( label, 2, 0 );

  mTitleWidget = new KLineEdit;
  label->setBuddy( mTitleWidget );
  generalLayout->addWidget( mTitleWidget , 2, 1 );

  label = new QLabel( i18n( "Department:" ) );
  label->setAlignment( Qt::AlignRight | Qt::AlignVCenter );
  generalLayout->addWidget( label, 3, 0 );

  mDepartmentWidget = new KLineEdit;
  label->setBuddy( mDepartmentWidget );
  generalLayout->addWidget( mDepartmentWidget, 3, 1 );

  label = new QLabel( i18n( "Office:" ) );
  label->setAlignment( Qt::AlignRight | Qt::AlignVCenter );
  generalLayout->addWidget( label, 4, 0 );

  mOfficeWidget = new KLineEdit;
  label->setBuddy( mOfficeWidget );
  generalLayout->addWidget( mOfficeWidget, 4, 1 );

  label = new QLabel( i18n( "Manager's name:" ) );
  label->setAlignment( Qt::AlignRight | Qt::AlignVCenter );
  generalLayout->addWidget( label, 5, 0 );

  mManagerWidget = new KLineEdit;
  label->setBuddy( mManagerWidget );
  generalLayout->addWidget( mManagerWidget, 5, 1 );

  label = new QLabel( i18n( "Assistant's name:" ) );
  label->setAlignment( Qt::AlignRight | Qt::AlignVCenter );
  generalLayout->addWidget( label, 6, 0 );

  mAssistantWidget = new KLineEdit;
  label->setBuddy( mAssistantWidget );
  generalLayout->addWidget( mAssistantWidget, 6, 1 );

  generalLayout->setRowStretch( 7, 1 );

  // setup groupware group box
  label = new QLabel( i18n( "Free/Busy:" ) );
  label->setAlignment( Qt::AlignRight | Qt::AlignVCenter );
  groupwareLayout->addWidget( label, 0, 0 );

  mFreeBusyWidget = new FreeBusyEditWidget;
  label->setBuddy( mFreeBusyWidget );
  groupwareLayout->addWidget( mFreeBusyWidget, 0, 1 );

  groupwareLayout->setRowStretch( 1, 1 );

  // setup notes group box
  mNotesWidget = new KTextEdit;
  notesLayout->addWidget( mNotesWidget, 0, 0 );
}

void ContactEditorWidget::Private::initGuiPersonalTab()
{
  QWidget *widget = new QWidget;
  QVBoxLayout *layout = new QVBoxLayout( widget );

  mTabWidget->addTab( widget, i18n( "Personal" ) );

  QGroupBox *datesGroupBox = new QGroupBox( i18n( "Dates" ) );
  QGroupBox *familyGroupBox = new QGroupBox( i18n( "Family" ) );

  layout->addWidget( datesGroupBox );
  layout->addWidget( familyGroupBox );

  QGridLayout *datesLayout = new QGridLayout( datesGroupBox );
  QGridLayout *familyLayout = new QGridLayout( familyGroupBox );

  QLabel *label = 0;

  // setup dates group box
  label = new QLabel( i18n( "Birthdate:" ) );
  label->setAlignment( Qt::AlignRight | Qt::AlignVCenter );
  datesLayout->addWidget( label, 0, 0 );

  mBirthdateWidget = new DateEditWidget;
  label->setBuddy( mBirthdateWidget );
  datesLayout->addWidget( mBirthdateWidget, 0, 1 );

  label = new QLabel( i18n( "Anniversary:" ) );
  label->setAlignment( Qt::AlignRight | Qt::AlignVCenter );
  datesLayout->addWidget( label, 1, 0 );

  mAnniversaryWidget = new DateEditWidget;
  label->setBuddy( mAnniversaryWidget );
  datesLayout->addWidget( mAnniversaryWidget, 1, 1 );

  datesLayout->setRowStretch( 2, 1 );
  datesLayout->setColumnStretch( 1, 1 );

  // widgets from family group
  label = new QLabel( i18n( "Partner's name:" ) );
  label->setAlignment( Qt::AlignRight | Qt::AlignVCenter );
  familyLayout->addWidget( label, 0, 0 );

  mPartnerWidget = new KLineEdit;
  label->setBuddy( mPartnerWidget );
  familyLayout->addWidget( mPartnerWidget, 0, 1 );

  familyLayout->setRowStretch( 1, 1 );
}

QString ContactEditorWidget::Private::loadCustom( const KABC::Addressee &contact, const QString &key ) const
{
  return contact.custom( QLatin1String( "KADDRESSBOOK" ), key );
}

void ContactEditorWidget::Private::storeCustom( KABC::Addressee &contact, const QString &key, const QString &value ) const
{
  if ( value.isEmpty() )
    contact.removeCustom( QLatin1String( "KADDRESSBOOK" ), key );
  else
    contact.insertCustom( QLatin1String( "KADDRESSBOOK" ), key, value );
}

ContactEditorWidget::ContactEditorWidget( QWidget* )
  : d( new Private( this ) )
{
  d->initGui();

  connect( d->mNameWidget, SIGNAL( nameChanged( const KABC::Addressee& ) ),
           d->mDisplayNameWidget, SLOT( changeName( const KABC::Addressee& ) ) );
  connect( d->mOrganizationWidget, SIGNAL( textChanged( const QString& ) ),
           d->mDisplayNameWidget, SLOT( changeOrganization( const QString& ) ) );
}

ContactEditorWidget::~ContactEditorWidget()
{
  delete d;
}

void ContactEditorWidget::loadContact( const KABC::Addressee &contact, const Akonadi::ContactMetaData &metaData )
{
  // name group
  d->mPhotoWidget->loadContact( contact );
  d->mNameWidget->loadContact( contact );
  d->mDisplayNameWidget->loadContact( contact );
  d->mNickNameWidget->setText( contact.nickName() );
  d->mPronunciationWidget->loadContact( contact );

  // internet group
  d->mEmailWidget->loadContact( contact );
  d->mHomepageWidget->setUrl( contact.url() );
  d->mBlogWidget->setText( d->loadCustom( contact, QLatin1String( "BlogFeed" ) ) );
  d->mIMWidget->loadContact( contact );

  // phones group
  d->mPhonesWidget->loadContact( contact );

  // address group
  d->mAddressesWidget->loadContact( contact );

  // coordinates group

  // general group
  d->mLogoWidget->loadContact( contact );
  d->mOrganizationWidget->setText( contact.organization() );
  d->mProfessionWidget->setText( d->loadCustom( contact, QLatin1String( "X-Profession" ) ) );
  d->mTitleWidget->setText( contact.title() );
  d->mDepartmentWidget->setText( contact.department() );
  d->mOfficeWidget->setText( d->loadCustom( contact, QLatin1String( "X-Office" ) ) );
  d->mManagerWidget->setText( d->loadCustom( contact, QLatin1String( "X-ManagersName" ) ) );
  d->mAssistantWidget->setText( d->loadCustom( contact, QLatin1String( "X-AssistantsName" ) ) );

  // groupware group
  d->mFreeBusyWidget->loadContact( contact );

  // notes group
  d->mNotesWidget->setPlainText( contact.note() );

  // dates group
  d->mBirthdateWidget->setDate( contact.birthday().date() );
  d->mAnniversaryWidget->setDate( QDate::fromString( d->loadCustom( contact, QLatin1String( "X-Anniversary" ) ),
                                                     Qt::ISODate ) );

  // family group
  d->mPartnerWidget->setText( d->loadCustom( contact, QLatin1String( "X-SpousesName" ) ) );

  d->mDisplayNameWidget->setDisplayType( (DisplayNameEditWidget::DisplayType)metaData.displayNameMode() );
}

void ContactEditorWidget::storeContact( KABC::Addressee &contact, Akonadi::ContactMetaData &metaData ) const
{
  // name group
  d->mPhotoWidget->storeContact( contact );
  d->mNameWidget->storeContact( contact );
  d->mDisplayNameWidget->storeContact( contact );
  contact.setNickName( d->mNickNameWidget->text().trimmed() );
  d->mPronunciationWidget->storeContact( contact );

  // internet group
  d->mEmailWidget->storeContact( contact );
  contact.setUrl( KUrl( d->mHomepageWidget->text().trimmed() ) );
  d->storeCustom( contact, QLatin1String( "BlogFeed" ), d->mBlogWidget->text().trimmed() );
  d->mIMWidget->storeContact( contact );

  // phones group
  d->mPhonesWidget->storeContact( contact );

  // address group
  d->mAddressesWidget->storeContact( contact );

  // coordinates group

  // general group
  d->mLogoWidget->storeContact( contact );
  contact.setOrganization( d->mOrganizationWidget->text() );
  d->storeCustom( contact, QLatin1String( "X-Profession" ), d->mProfessionWidget->text().trimmed() );
  contact.setTitle( d->mTitleWidget->text().trimmed() );
  contact.setDepartment( d->mDepartmentWidget->text().trimmed() );
  d->storeCustom( contact, QLatin1String( "X-Office" ), d->mOfficeWidget->text().trimmed() );
  d->storeCustom( contact, QLatin1String( "X-ManagersName" ), d->mManagerWidget->text().trimmed() );
  d->storeCustom( contact, QLatin1String( "X-AssistantsName" ), d->mAssistantWidget->text().trimmed() );

  // groupware group
  d->mFreeBusyWidget->storeContact( contact );

  // notes group
  contact.setNote( d->mNotesWidget->toPlainText() );

  // dates group
  contact.setBirthday( QDateTime( d->mBirthdateWidget->date(), QTime() ) );
  d->storeCustom( contact, QLatin1String( "X-Anniversary" ), d->mAnniversaryWidget->date().toString( Qt::ISODate ) );

  // family group
  d->storeCustom( contact, QLatin1String( "X-SpousesName" ), d->mPartnerWidget->text().trimmed() );

  metaData.setDisplayNameMode( d->mDisplayNameWidget->displayType() );
}

void ContactEditorWidget::setReadOnly( bool readOnly )
{
  // widgets from name group
  d->mNameWidget->setReadOnly( readOnly );
  d->mPhotoWidget->setReadOnly( readOnly );
  d->mDisplayNameWidget->setReadOnly( readOnly );
  d->mNickNameWidget->setReadOnly( readOnly );
  d->mPronunciationWidget->setReadOnly( readOnly );

  // widgets from internet group
  d->mEmailWidget->setReadOnly( readOnly );
  d->mHomepageWidget->setReadOnly( readOnly );
  d->mBlogWidget->setReadOnly( readOnly );
  d->mIMWidget->setReadOnly( readOnly );

  // widgets from phones group
  d->mPhonesWidget->setReadOnly( readOnly );

  // widgets from addresses group
  d->mAddressesWidget->setReadOnly( readOnly );

  // widgets from coordinates group
  d->mCoordinatesWidget->setEnabled( false );

  // widgets from general group
  d->mLogoWidget->setReadOnly( readOnly );
  d->mOrganizationWidget->setReadOnly( readOnly );
  d->mProfessionWidget->setReadOnly( readOnly );
  d->mTitleWidget->setReadOnly( readOnly );
  d->mDepartmentWidget->setReadOnly( readOnly );
  d->mOfficeWidget->setReadOnly( readOnly );
  d->mManagerWidget->setReadOnly( readOnly );
  d->mAssistantWidget->setReadOnly( readOnly );

  // widgets from groupware group
  d->mFreeBusyWidget->setReadOnly( readOnly );

  // widgets from notes group
  d->mNotesWidget->setReadOnly( readOnly );

  // widgets from dates group
  d->mBirthdateWidget->setReadOnly( readOnly );
  d->mAnniversaryWidget->setReadOnly( readOnly );

  // widgets from family group
  d->mPartnerWidget->setReadOnly( readOnly );
}
