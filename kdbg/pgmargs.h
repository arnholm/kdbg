// $Id$

// Copyright by Johannes Sixt
// This file is under GPL, the GNU General Public Licence

#ifndef PgmArgs_included
#define PgmArgs_included

#include <qdialog.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qpushbutton.h>
#include <qlayout.h>
#include <qlistview.h>
#include <qlistbox.h>
#include <qdict.h>
#include "envvar.h"

class QStringList;

class PgmArgs : public QDialog
{
    Q_OBJECT
public:
    PgmArgs(QWidget* parent, const QString& pgm, QDict<EnvVar>& envVars,
	    const QStringList& allOptions);
    virtual ~PgmArgs();

    void setArgs(const QString& text) { m_programArgs.setText(text); }
    const char* args() const { return m_programArgs.text(); }
    void setOptions(const QStringList& selectedOptions);
    QStringList options() const;
    void setWd(const QString& wd) { m_wd.setText(wd); }
    QString wd() const { return m_wd.text(); }
    QDict<EnvVar>& envVars() { return m_envVars; }

protected:
    QDict<EnvVar> m_envVars;

    void initEnvList();
    void parseEnvInput(QString& name, QString& value);

    QLabel m_label;
    QLineEdit m_programArgs;
    QPushButton m_fileBrowse;
    QLabel m_optionsLabel;
    QListBox m_options;
    QLabel m_wdLabel;
    QLineEdit m_wd;
    QPushButton m_wdBrowse;
    QLabel m_envLabel;
    QLineEdit m_envVar;
    QListView m_envList;
    QPushButton m_buttonOK;
    QPushButton m_buttonCancel;
    QPushButton m_buttonModify;
    QPushButton m_buttonDelete;
    QHBoxLayout m_layout;
    QVBoxLayout m_edits;
    QVBoxLayout m_buttons;
    QHBoxLayout m_pgmArgsEdit;
    QHBoxLayout m_wdEdit;

protected slots:
    void modifyVar();
    void deleteVar();
    void envListCurrentChanged(QListViewItem*);
    void accept();
    void browseWd();
    void browseArgs();
};

#endif // PgmArgs_included
