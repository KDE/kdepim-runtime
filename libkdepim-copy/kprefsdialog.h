/*
    This file is part of libkdepim.

    Copyright (c) 2001-2003 Cornelius Schumacher <schumacher@kde.org>
    Copyright (C) 2003-2004 Reinhold Kainhofer <reinhold@kainhofer.com>

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
#ifndef KPREFSDIALOG_H
#define KPREFSDIALOG_H

#include <qptrlist.h>
#include <qlineedit.h>
#include <qvaluelist.h>

#include <kdialogbase.h>
#include <kcmodule.h>
#include <kconfigskeleton.h>
#include <kfile.h>


class KColorButton;
class QCheckBox;
class QLabel;
class QSpinBox;
class QButtonGroup;
class KTimeEdit;
class KDateEdit;
class KURLRequester;

/**
  @short Base class for GUI control elements used by @ref KPrefsDialog.
  @author Cornelius Schumacher
  @see KPrefsDialog

  This class provides the interface for the GUI control elements used by
  KPrefsDialog. The control element consists of a set of widgets for handling
  a certain type of configuration information.
*/
class KPrefsWid : public QObject
{
    Q_OBJECT
  public:
    /**
      This function is called to read value of the setting from the
      stored configuration and display it in the widget.
    */
    virtual void readConfig() = 0;
    /**
      This function is called to write the current setting of the widget to the
      stored configuration.
    */
    virtual void writeConfig() = 0;

    /**
      Return a list of widgets used by this control element.
    */
    virtual QValueList<QWidget *> widgets() const;

  signals:
    /**
      Emitted when widget value has changed.
    */
    void changed();
};

/**
  @short Widgets for bool settings in @ref KPrefsDialog.

  This class provides a control element for configuring bool values. It is meant
  to be used by KPrefsDialog. The user is responsible for the layout management.
*/
class KPrefsWidBool : public KPrefsWid
{
  public:
    /**
      Create a bool value control element consisting of a QCheckbox.

      @param item    The KConfigSkeletonItem representing the preferences entry.
      @param parent  Parent widget.
    */
    KPrefsWidBool( KConfigSkeleton::ItemBool *item, QWidget *parent );

    /**
      Return the QCheckbox used by this control element.
    */
    QCheckBox *checkBox();

    void readConfig();
    void writeConfig();

    QValueList<QWidget *> widgets() const;

  private:
    KConfigSkeleton::ItemBool *mItem;

    QCheckBox *mCheck;
};

/**
  @short Widgets for int settings in @ref KPrefsDialog.

  This class provides a control element for configuring integer values. It is
  meant to be used by KPrefsDialog. The user is responsible for the layout
  management.
*/
class KPrefsWidInt : public KPrefsWid
{
  public:
    /**
      Create a integer value control element consisting of a label and a
      spinbox.

      @param item    The KConfigSkeletonItem representing the preferences entry.
      @param parent  Parent widget.
    */
    KPrefsWidInt( KConfigSkeleton::ItemInt *item, QWidget *parent );

    /**
      Return QLabel used by this control element.
    */
    QLabel *label();

    /**
      Return the QSpinBox used by this control element.
    */
    QSpinBox *spinBox();

    void readConfig();
    void writeConfig();

    QValueList<QWidget *> widgets() const;

  private:
    KConfigSkeleton::ItemInt *mItem;

    QLabel *mLabel;
    QSpinBox *mSpin;
};

/**
  @short Widgets for time settings in @ref KPrefsDialog.

  This class provides a control element for configuring time values. It is
  meant to be used by KPrefsDialog. The user is responsible for the layout
  management.
*/
class KPrefsWidTime : public KPrefsWid
{
  public:
    /**
      Create a time value control element consisting of a label and a spinbox.

      @param item    The KConfigSkeletonItem representing the preferences entry.
      @param parent  Parent widget.
    */
    KPrefsWidTime( KConfigSkeleton::ItemDateTime *item, QWidget *parent );

    /**
      Return QLabel used by this widget.
    */
    QLabel *label();
    /**
      Return QSpinBox used by this widget.
    */
    KTimeEdit *timeEdit();

    void readConfig();
    void writeConfig();

  private:
    KConfigSkeleton::ItemDateTime *mItem;

    QLabel *mLabel;
    KTimeEdit *mTimeEdit;
};

/**
  @short Widgets for time settings in @ref KPrefsDialog.

  This class provides a control element for configuring date values. It is
  meant to be used by KPrefsDialog. The user is responsible for the layout
  management.
*/
class KPrefsWidDate : public KPrefsWid
{
  public:
    /**
      Create a time value control element consisting of a label and a date box.

      @param item    The KConfigSkeletonItem representing the preferences entry.
      @param parent  Parent widget.
    */
    KPrefsWidDate( KConfigSkeleton::ItemDateTime *item, QWidget *parent );

    /**
      Return QLabel used by this widget.
    */
    QLabel *label();
    /**
      Return QSpinBox used by this widget.
    */
    KDateEdit *dateEdit();

    void readConfig();
    void writeConfig();

  private:
    KConfigSkeleton::ItemDateTime *mItem;

    QLabel *mLabel;
    KDateEdit *mDateEdit;
};

/**
  @short Widgets for color settings in @ref KPrefsDialog.

  This class provides a control element for configuring color values. It is
  meant to be used by KPrefsDialog. The user is responsible for the layout
  management.
*/
class KPrefsWidColor : public KPrefsWid
{
    Q_OBJECT
  public:
    /**
      Create a color value control element consisting of a test field and a
      button for opening a color dialog.

      @param item    The KConfigSkeletonItem representing the preferences entry.
      @param parent  Parent widget.
    */
    KPrefsWidColor( KConfigSkeleton::ItemColor *item, QWidget *parent );
    /**
      Destruct color setting widget.
    */
    ~KPrefsWidColor();

    /**
      Return QLabel for the button
    */
    QLabel *label();
    /**
      Return button opening the color dialog.
    */
    KColorButton *button();

    void readConfig();
    void writeConfig();

  private:
    KConfigSkeleton::ItemColor *mItem;

    QLabel *mLabel;
    KColorButton *mButton;
};

/**
  @short Widgets for font settings in @ref KPrefsDialog.

  This class provides a control element for configuring font values. It is meant
  to be used by KPrefsDialog. The user is responsible for the layout management.
*/
class KPrefsWidFont : public KPrefsWid
{
    Q_OBJECT
  public:
    /**
      Create a font value control element consisting of a test field and a
      button for opening a font dialog.

      @param item    The KConfigSkeletonItem representing the preferences entry.
      @param parent  Parent widget.
      @param sampleText Sample text for previewing the selected font.
    */
    KPrefsWidFont( KConfigSkeleton::ItemFont *item,
                   QWidget *parent, const QString &sampleText );
    /**
      Destruct font setting widget.
    */
    ~KPrefsWidFont();

    /**
      Return QLabel.
    */
    QLabel *label();
    /**
      Return QFrame used as preview field.
    */
    QFrame *preview();
    /**
      Return button opening the font dialog.
    */
    QPushButton *button();

    void readConfig();
    void writeConfig();

  protected slots:
    void selectFont();

  private:
    KConfigSkeleton::ItemFont *mItem;

    QLabel *mLabel;
    QLabel *mPreview;
    QPushButton *mButton;
};

/**
  @short Widgets for settings represented by a group of radio buttons in
  @ref KPrefsDialog.

  This class provides a control element for configuring selections. It is meant
  to be used by KPrefsDialog. The user is responsible for the layout management.

  The setting is interpreted as an int value, corresponding to the position of
  the radio button. The position of the button is defined by the sequence of
  @ref addRadio() calls, starting with 0.
*/
class KPrefsWidRadios : public KPrefsWid
{
  public:
    /**
      Create a control element for selection of an option. It consists of a box
      with several radio buttons.

      @param item    The KConfigSkeletonItem representing the preferences entry.
      @param parent  Parent widget.
    */
    KPrefsWidRadios( KConfigSkeleton::ItemEnum *item, QWidget *parent );
    virtual ~KPrefsWidRadios();

    /**
      Add a radio button.

      @param text Text of the button.
      @param whatsThis What's This help for the button.
    */
    void addRadio( const QString &text,
                   const QString &whatsThis = QString::null );

    /**
      Return the box widget used by this widget.
    */
    QButtonGroup *groupBox();

    void readConfig();
    void writeConfig();

    QValueList<QWidget *> widgets() const;

  private:
    KConfigSkeleton::ItemEnum *mItem;

    QButtonGroup *mBox;
};


/**
  @short Widgets for string settings in @ref KPrefsDialog.

  This class provides a control element for configuring string values. It is
  meant to be used by KPrefsDialog. The user is responsible for the layout
  management.
*/
class KPrefsWidString : public KPrefsWid
{
  public:
    /**
      Create a string value control element consisting of a test label and a
      line edit.

      @param item    The KConfigSkeletonItem representing the preferences entry.
      @param parent  Parent widget.
      @param echomode  Describes how a line edit should display its contents.
    */
    KPrefsWidString( KConfigSkeleton::ItemString *item, QWidget *parent,
                     QLineEdit::EchoMode echomode=QLineEdit::Normal );
    /**
      Destructor.
    */
    virtual ~KPrefsWidString();

    /**
      Return QLabel used by this widget.
    */
    QLabel *label();
    /**
      Return QLineEdit used by this widget.
    */
    QLineEdit *lineEdit();

    void readConfig();
    void writeConfig();

    QValueList<QWidget *> widgets() const;

  private:
    KConfigSkeleton::ItemString *mItem;

    QLabel *mLabel;
    QLineEdit *mEdit;
};


/**
  @short Widgets for string settings in @ref KPrefsDialog.

  This class provides a control element for configuring string values. It is
  meant to be used by KPrefsDialog. The user is responsible for the layout
  management.
*/
class KPrefsWidPath : public KPrefsWid
{
  public:
    /**
      Create a string value control element consisting of a test label and a
      line edit.

      @param item    The KConfigSkeletonItem representing the preferences entry.
      @param parent  Parent widget.
      @param filter URLRequester filter
      @param mode  Describes how a line edit should display its contents.
    */
    KPrefsWidPath( KConfigSkeleton::ItemPath *item, QWidget *parent,
                   const QString &filter = QString::null, uint mode = KFile::File );
    /**
      Destructor.
    */
    virtual ~KPrefsWidPath();

    /**
      Return QLabel used by this widget.
    */
    QLabel *label();
    /**
      Return QLineEdit used by this widget.
    */
    KURLRequester *urlRequester();

    void readConfig();
    void writeConfig();

    QValueList<QWidget *> widgets() const;

  private:
    KConfigSkeleton::ItemPath *mItem;

    QLabel *mLabel;
    KURLRequester *mURLRequester;
};


/**
  @short Class for managing KPrefsWid objects.

  This class manages standard configuration widgets provided bz the KPrefsWid
  subclasses. It handles creation, loading, saving and default values in a
  transparent way. The user has to add the widgets by the corresponding addWid
  functions and KPrefsWidManager handles the rest automatically.
*/
class KPrefsWidManager
{
  public:
    /**
      Create a KPrefsWidManager object for a KPrefs object.

      @param prefs  KPrefs object used to access te configuration.
    */
    KPrefsWidManager( KConfigSkeleton *prefs );
    /**
      Destructor.
    */
    virtual ~KPrefsWidManager();

    KConfigSkeleton *prefs() const { return mPrefs; }

    /**
      Register a custom KPrefsWid object.
    */
    virtual void addWid( KPrefsWid * );

    /**
      Register a @ref KPrefsWidBool object.

      @param item    The KConfigSkeletonItem representing the preferences entry.
      @param parent  Parent widget.
    */
    KPrefsWidBool *addWidBool( KConfigSkeleton::ItemBool *item,
                               QWidget *parent );

    /**
      Register a @ref KPrefsWidInt object.

      @param item    The KConfigSkeletonItem representing the preferences entry.
      @param parent  Parent widget.
    */
    KPrefsWidInt *addWidInt( KConfigSkeleton::ItemInt *item,
                             QWidget *parent );

    /**
      Register a @ref KPrefsWidDate object.

      @param item    The KConfigSkeletonItem representing the preferences entry.
      @param parent  Parent widget.
    */
    KPrefsWidDate *addWidDate( KConfigSkeleton::ItemDateTime *item,
                               QWidget *parent );

    /**
      Register a @ref KPrefsWidTime object.

      @param item    The KConfigSkeletonItem representing the preferences entry.
      @param parent  Parent widget.
    */
    KPrefsWidTime *addWidTime( KConfigSkeleton::ItemDateTime *item,
                               QWidget *parent );

    /**
      Register a @ref KPrefsWidColor object.

      @param item    The KConfigSkeletonItem representing the preferences entry.
      @param parent  Parent widget.
    */
    KPrefsWidColor *addWidColor( KConfigSkeleton::ItemColor *item,
                                 QWidget *parent );

    /**
      Register a @ref KPrefsWidRadios object. The choices represented by the
      given item object are automatically added as radio buttons.

      @param item    The KConfigSkeletonItem representing the preferences entry.
      @param parent  Parent widget.
    */
    KPrefsWidRadios *addWidRadios( KConfigSkeleton::ItemEnum *item,
                                   QWidget *parent );

    /**
      Register a @ref KPrefsWidString object.

      @param item    The KConfigSkeletonItem representing the preferences entry.
      @param parent  Parent widget.
    */
    KPrefsWidString *addWidString( KConfigSkeleton::ItemString *item,
                                   QWidget *parent );

    /**
      Register a path @ref KPrefsWidPath object.

      @param item    The KConfigSkeletonItem representing the preferences entry.
      @param parent  Parent widget.
      @param filter URLRequester filter
      @param mode URLRequester mode
    */
    KPrefsWidPath *addWidPath ( KConfigSkeleton::ItemPath *item, QWidget *parent,
                                const QString &filter = QString::null,
                                uint mode = KFile::File );

    /**
      Register a password @ref KPrefsWidString object, with echomode set to QLineEdit::Password.

      @param item    The KConfigSkeletonItem representing the preferences entry.
      @param parent  Parent widget.
    */
    KPrefsWidString *addWidPassword ( KConfigSkeleton::ItemString *item,
                                      QWidget *parent );

    /**
      Register a @ref KPrefsWidFont object.

      @param item       The KConfigSkeletonItem representing the preferences
                        entry.
      @param parent     Parent widget.
      @param sampleText Sample text for previewing the selected font.
    */
    KPrefsWidFont *addWidFont( KConfigSkeleton::ItemFont *item,
                               QWidget *parent, const QString &sampleText );

    /** Set all widgets to default values. */
    void setWidDefaults();

    /** Read preferences from config file. */
    void readWidConfig();

    /** Write preferences to config file. */
    void writeWidConfig();

  private:
    KConfigSkeleton *mPrefs;

    QPtrList<KPrefsWid> mPrefsWids;
};


/**
  @short Base class for a preferences dialog.

  This class provides the framework for a preferences dialog. You have to
  subclass it and add the code to create the actual configuration widgets and
  do the layout management.

  KPrefsDialog provides functions to add subclasses of @ref KPrefsWid via
  KPrefsWidManager. For these widgets the reading, writing and setting to
  default values is handled automatically. Custom widgets have to be handled in
  the functions @ref usrReadConfig() and @ref usrWriteConfig().
*/
class KPrefsDialog : public KDialogBase, public KPrefsWidManager
{
    Q_OBJECT
  public:
    /**
      Create a KPrefsDialog for a KPrefs object.

      @param prefs  KPrefs object used to access te configuration.
      @param parent Parent widget.
      @param name   Widget name.
      @param modal  true, if dialog has to be modal, false for non-modal.
    */
    KPrefsDialog( KConfigSkeleton *prefs, QWidget *parent = 0, char *name = 0,
                  bool modal = false );
    /**
      Destructor.
    */
    virtual ~KPrefsDialog();

    void autoCreate();

  public slots:
    /** Set all widgets to default values. */
    void setDefaults();

    /** Read preferences from config file. */
    void readConfig();

    /** Write preferences to config file. */
    void writeConfig();

  signals:
    /** Emitted when the a changed configuration has been stored. */
    void configChanged();

  protected slots:
    /** Apply changes to preferences */
    void slotApply();

    /** Accept changes to preferences and close dialog */
    void slotOk();

    /** Set preferences to default values */
    void slotDefault();

  protected:
    /** Implement this to read custom configuration widgets. */
    virtual void usrReadConfig() {}
    /** Implement this to write custom configuration widgets. */
    virtual void usrWriteConfig() {}
};


class KPrefsModule : public KCModule, public KPrefsWidManager
{
    Q_OBJECT
  public:
    KPrefsModule( KConfigSkeleton *, QWidget *parent = 0, const char *name = 0 );

    virtual void addWid( KPrefsWid * );

    void load();
    void save();
    void defaults();

  protected slots:
    void slotWidChanged();

  protected:
    /** Implement this to read custom configuration widgets. */
    virtual void usrReadConfig() {}
    /** Implement this to write custom configuration widgets. */
    virtual void usrWriteConfig() {}
};

#endif
