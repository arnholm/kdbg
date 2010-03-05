/*
 * Copyright Johannes Sixt
 * This file is licensed under the GNU General Public License Version 2.
 * See the file COPYING in the toplevel directory of the source directory.
 */

#include "procattach.h"
#include <Q3ListView>
#include <k3process.h>
#include <ctype.h>
#include <kglobal.h>
#include <kiconloader.h>
#include <klocale.h>			/* i18n */
#include "config.h"


ProcAttachPS::ProcAttachPS(QWidget* parent) :
	KDialog(parent),
	m_pidCol(-1),
	m_ppidCol(-1)
{
    setupUi(this);

    m_ps = new K3Process;
    connect(m_ps, SIGNAL(receivedStdout(K3Process*, char*, int)),
	    this, SLOT(slotTextReceived(K3Process*, char*, int)));
    connect(m_ps, SIGNAL(processExited(K3Process*)),
	    this, SLOT(slotPSDone()));

    QIcon icon = SmallIconSet("clear_left");
    filterClear->setIconSet(icon);

    processList->setColumnWidth(0, 300);
    processList->setColumnWidthMode(0, Q3ListView::Manual);
    processList->setColumnAlignment(1, Qt::AlignRight);
    processList->setColumnAlignment(2, Qt::AlignRight);

    // set the command line
    static const char* const psCommand[] = {
#ifdef PS_COMMAND
	PS_COMMAND,
#else
	"/bin/false",
#endif
	0
    };
    for (int i = 0; psCommand[i] != 0; i++) {
	*m_ps << psCommand[i];
    }

    runPS();
}

ProcAttachPS::~ProcAttachPS()
{
    delete m_ps;	// kills a running ps
}

void ProcAttachPS::runPS()
{
    // clear the parse state from previous runs
    m_token = "";
    m_line.clear();
    m_pidCol = -1;
    m_ppidCol = -1;

    m_ps->start(K3Process::NotifyOnExit, K3Process::Stdout);
}

void ProcAttachPS::slotTextReceived(K3Process*, char* buffer, int buflen)
{
    const char* end = buffer+buflen;
    while (buffer < end)
    {
	// check new line
	if (*buffer == '\n')
	{
	    // push a tokens onto the line
	    if (!m_token.isEmpty()) {
		m_line.push_back(QString::fromLatin1(m_token));
		m_token = "";
	    }
	    // and insert the line in the list
	    pushLine();
	    m_line.clear();
	    ++buffer;
	}
	// blanks: the last column gets the rest of the line, including blanks
	else if ((m_pidCol < 0 || int(m_line.size()) < processList->columns()-1) &&
		 isspace(*buffer))
	{
	    // push a token onto the line
	    if (!m_token.isEmpty()) {
		m_line.push_back(QString::fromLatin1(m_token));
		m_token = "";
	    }
	    do {
		++buffer;
	    } while (buffer < end && isspace(*buffer));
	}
	// tokens
	else
	{
	    const char* start = buffer;
	    do {
		++buffer;
	    } while (buffer < end && !isspace(*buffer));
	    // append to the current token
	    m_token += Q3CString(start, buffer-start+1);	// must count the '\0'
	}
    }
}

void ProcAttachPS::pushLine()
{
    if (m_line.size() < 3)	// we need the PID, PPID, and COMMAND columns
	return;

    if (m_pidCol < 0)
    {
	// create columns if we don't have them yet
	bool allocate =	processList->columns() == 3;

	// we assume that the last column is the command
	m_line.pop_back();

	for (int i = 0; i < m_line.size(); i++) {
	    // we don't allocate the PID and PPID columns,
	    // but we need to know where in the ps output they are
	    if (m_line[i] == "PID") {
		m_pidCol = i;
	    } else if (m_line[i] == "PPID") {
		m_ppidCol = i;
	    } else if (allocate) {
		processList->addColumn(m_line[i]);
		// these columns are normally numbers
		processList->setColumnAlignment(processList->columns()-1,
						Qt::AlignRight);
	    }
	}
    }
    else
    {
	// insert a line
	// find the parent process
	Q3ListViewItem* parent = 0;
	if (m_ppidCol >= 0 && m_ppidCol < int(m_line.size())) {
	    parent = processList->findItem(m_line[m_ppidCol], 1);
	}

	// we assume that the last column is the command
	Q3ListViewItem* item;
	if (parent == 0) {
	    item = new Q3ListViewItem(processList, m_line.back());
	} else {
	    item = new Q3ListViewItem(parent, m_line.back());
	}
	item->setOpen(true);
	m_line.pop_back();
	int k = 3;
	for (int i = 0; i < m_line.size(); i++)
	{
	    // display the pid and ppid columns' contents in columns 1 and 2
	    if (int(i) == m_pidCol)
		item->setText(1, m_line[i]);
	    else if (int(i) == m_ppidCol)
		item->setText(2, m_line[i]);
	    else
		item->setText(k++, m_line[i]);
	}

	if (m_ppidCol >= 0 && m_pidCol >= 0) {	// need PID & PPID for this
	    /*
	     * It could have happened that a process was earlier inserted,
	     * whose parent process is the current process. Such processes
	     * were placed at the root. Here we go through all root items
	     * and check whether we must reparent them.
	     */
	    Q3ListViewItem* i = processList->firstChild();
	    while (i != 0)
	    {
		// advance before we reparent the item
		Q3ListViewItem* it = i;
		i = i->nextSibling();
		if (it->text(2) == m_line[m_pidCol]) {
		    processList->takeItem(it);
		    item->insertItem(it);
		}
	    }
	}
    }
}

void ProcAttachPS::slotPSDone()
{
    on_filterEdit_textChanged(filterEdit->text());
}

QString ProcAttachPS::text() const
{
    Q3ListViewItem* item = processList->selectedItem();

    if (item == 0)
	return QString();

    return item->text(1);
}

void ProcAttachPS::on_buttonRefresh_clicked()
{
    if (!m_ps->isRunning())
    {
	processList->clear();
	buttonOk->setEnabled(false);	// selection was cleared
	runPS();
    }
}

void ProcAttachPS::on_filterEdit_textChanged(const QString& text)
{
    Q3ListViewItem* i = processList->firstChild();
    if (i) {
	setVisibility(i, text);
    }
}

/**
 * Sets the visibility of \a i and
 * returns whether it was made visible.
 */
bool ProcAttachPS::setVisibility(Q3ListViewItem* i, const QString& text)
{
    bool visible = false;
    for (Q3ListViewItem* j = i->firstChild(); j; j = j->nextSibling())
    {
	if (setVisibility(j, text))
	    visible = true;
    }
    // look for text in the process name and in the PID
    visible = visible || text.isEmpty() ||
	i->text(0).find(text, 0, false) >= 0 ||
	i->text(1).find(text) >= 0;

    i->setVisible(visible);

    // disable the OK button if the selected item becomes invisible
    if (i->isSelected())
	buttonOk->setEnabled(visible);

    return visible;
}

void ProcAttachPS::on_processList_selectionChanged()
{
    buttonOk->setEnabled(processList->selectedItem() != 0);
}


ProcAttach::ProcAttach(QWidget* parent) :
	QDialog(parent, "procattach", true),
	m_label(this, "label"),
	m_processId(this, "procid"),
	m_buttonOK(this, "ok"),
	m_buttonCancel(this, "cancel"),
	m_layout(this, 8),
	m_buttons(4)
{
    QString title = KGlobal::caption();
    title += i18n(": Attach to process");
    setCaption(title);

    m_label.setMinimumSize(330, 24);
    m_label.setText(i18n("Specify the process number to attach to:"));

    m_processId.setMinimumSize(330, 24);
    m_processId.setMaxLength(100);
    m_processId.setFrame(true);

    m_buttonOK.setMinimumSize(100, 30);
    connect(&m_buttonOK, SIGNAL(clicked()), SLOT(accept()));
    m_buttonOK.setText(i18n("OK"));
    m_buttonOK.setDefault(true);

    m_buttonCancel.setMinimumSize(100, 30);
    connect(&m_buttonCancel, SIGNAL(clicked()), SLOT(reject()));
    m_buttonCancel.setText(i18n("Cancel"));

    m_layout.addWidget(&m_label);
    m_layout.addWidget(&m_processId);
    m_layout.addLayout(&m_buttons);
    m_layout.addStretch(10);
    m_buttons.addStretch(10);
    m_buttons.addWidget(&m_buttonOK);
    m_buttons.addSpacing(40);
    m_buttons.addWidget(&m_buttonCancel);
    m_buttons.addStretch(10);

    m_layout.activate();

    m_processId.setFocus();
    resize(350, 120);
}

ProcAttach::~ProcAttach()
{
}


#include "procattach.moc"
