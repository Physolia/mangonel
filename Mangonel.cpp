#include "Mangonel.h"

#include <QVBoxLayout>
#include <QDesktopWidget>
#include <QTimeLine>
#include <QGraphicsItemAnimation>
#include <KDE/KAction>
#include <QDBusInterface>

//Include the providers.
#include "Applications.h"
#include "Paths.h"
#include "Shell.h"


int WIDTH = 220;
int HEIGHT = 200;

Mangonel::Mangonel(QApplication* app)
{
    this->setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    this->app = app;
    this->processingKey = false;
    this->apps = 0;
    QVBoxLayout* view = new QVBoxLayout(this);
    this->setLayout(view);
    // Setup the search feedback label.
    this->label = new Label(this);
    // Instantiate the visual feedback field.
    this->iconView = new IconView(this);
    // Add all to our layout.
    view->addWidget(this->iconView);
    view->addWidget(this->label);
    this->resize(WIDTH, HEIGHT);
    this->label->setMaximumWidth(WIDTH - 42);

    // Setup our global shortcut.
    KAction* action = new KAction(QString("Show Mangonel"), this);
    action->setObjectName(QString("show"));
    KShortcut shortcut = action->shortcut();
    shortcut.setPrimary(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_Space));
    action->setGlobalShortcut(shortcut);
    this->connect(action, SIGNAL(triggered(bool)), this, SLOT(showHide(bool)));

    // Instantiate the providers.
    this->providers["applications"] = new Applications();
    this->providers["paths"] = new Paths();
    this->providers["shell"] = new Shell();
}

Mangonel::~Mangonel()
{}

void Mangonel::keyPressEvent(QKeyEvent* event)
{
    int key = event->key();
    QString direction = "right";
    Application* CurrentApp;
    if (this->processingKey)
        return;
    this->processingKey = true;
    switch (event->key())
    {
    case Qt::Key_Enter:
    case Qt::Key_Return:
        this->launch();
    case Qt::Key_Escape:
        this->showHide();
        break;
    case Qt::Key_Left:
        direction = "left";
    case Qt::Key_Right:
        this->iconView->moveItems(direction);
        CurrentApp = this->iconView->selectedApp();
        if (CurrentApp != 0)
            this->label->setCompletion(CurrentApp->completion);
        break;
    default:
        if (key == Qt::Key_Tab)
        {
            if (!this->label->completion().isEmpty())
                this->label->setText(this->label->completion());
        }
        else if (key == Qt::Key_Backspace)
        {
            QString text = this->label->text();
            text.chop(1);
            if (text == "~/")
                text = "";
            this->label->setText(text);
        }
        else
        {
            this->label->setText(this->label->text().append(event->text()));
        }
        this->getApp(this->label->text());
    }
    this->processingKey = false;
}

void Mangonel::getApp(QString query)
{
    this->iconView->clear();
    delete this->apps;
    this->apps = 0;
    if (query.length() > 0)
    {
        this->apps = new AppList();
        this->current = -1;
        foreach(Provider* provider, this->providers)
        {
            AppList list = provider->getResults(query);
            foreach(Application app, list)
            this->apps->insertSorted(app);
        }
        if (!this->apps->isEmpty())
        {
            for (int i = 0; i < this->apps->length(); i++)
            {
                this->iconView->addProgram(this->apps->at(i));
            }
            this->iconView->setFirst();
            Application* CurrentApp = this->iconView->selectedApp();
            if (CurrentApp != 0)
                this->label->setCompletion(CurrentApp->completion);
        }
        else
        {
            this->label->setCompletion("");
        }
    }
}

void Mangonel::launch()
{
    Application* app = this->iconView->selectedApp();
    if (app != 0)
        app->object->launch(app->program);
}

void Mangonel::showHide(bool type)
{
    if (this->isVisible())
    {
        this->label->setText("");
        this->iconView->clear();
        delete this->apps;
        this->apps = 0;
        this->hide();
    }
    else
    {
        this->resize(WIDTH, HEIGHT);
        QRect screen = this->app->desktop()->screenGeometry(this);
        int x = (screen.width() - this->geometry().width()) / 2;
        int y = (screen.height() - this->geometry().height()) / 2;
        this->move(x, y);
        this->show();
        this->activateWindow();
        this->setFocus();
    }
}

void Mangonel::focusOutEvent(QFocusEvent* event)
{
    this->showHide();
}


IconView::IconView(QWidget* parent) : current(-1)
{
    this->scene = new QGraphicsScene(QRectF(0, 0, this->rect().width()*4, this->rect().height()), this);
    this->setScene(this->scene);
    this->setFrameStyle(QFrame::NoFrame);
    this->setStyleSheet("background: transparent; border: none");
    this->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    this->setFocusPolicy(Qt::NoFocus);
    this->centerOn(QPoint(this->rect().width()*1.5, 0));
}

IconView::~IconView()
{
    delete this->scene;
}

void IconView::clear()
{
    this->scene->clear();
    this->items.clear();
    this->current = -1;
}

void IconView::addProgram(Application application)
{
    ProgramView* program = new ProgramView(application);
    this->items.append(program);
    this->scene->addItem(program);
}

Application* IconView::selectedApp()
{
    if (this->current >= 0 and this->current < this->items.length())
    {
        return &this->items[this->current]->application;
    }
    else return 0;
}

void IconView::setFirst()
{
    if (!this->items.empty())
        this->current = 0;
    this->items[this->current]->show();
    this->items[this->current]->setPos(this->rect().width() + (this->rect().width() - 128) / 2, 0);
    this->centerOn(QPoint(this->rect().width()*1.5, 0));
}

void IconView::moveItems(QString direction)
{
    if (this->current < 0)
        return;
    int offset = this->rect().width();
    int steps =  10;
    int dx = offset / steps;
    int index = 1;
    if (direction == "right")
    {
        if (this->current + 1 >= this->items.length())
            return;
        dx = -dx;
        offset *= 2;
    }
    else
    {
        if (this->current < 1)
            return;
        offset = 0;
        index = -1;
    }
    ProgramView* itemOld = this->items[this->current];
    ProgramView* itemNew = this->items[this->current+index];
    itemNew->setPos(offset + (this->rect().width() - 128) / 2, 0);
    itemNew->show();
    int startposNew = itemNew->pos().x();
    int startPosOld = itemOld->pos().x();
    for (int i = 0; i < steps / 2; i++)
    {
        itemNew->setPos(startposNew + (dx * i), 0);
        QApplication::instance()->processEvents();
        usleep(5000);
    }
    startposNew = itemNew->pos().x();
    startPosOld = itemOld->pos().x();
    for (int i = 0; i < steps / 2; i++)
    {
        itemNew->setPos(startposNew + (dx * i), 0);
        itemOld->setPos(startPosOld + (dx * i), 0);
        QApplication::instance()->processEvents();
        usleep(5000);
    }
    startposNew = itemNew->pos().x();
    startPosOld = itemOld->pos().x();
    for (int i = 0; i < steps / 2; i++)
    {
        itemOld->setPos(startPosOld + (dx * i), 0);
        QApplication::instance()->processEvents();
        usleep(5000);
    }
    itemOld->hide();
    itemNew->setPos(this->rect().width() + (this->rect().width() - 128) / 2, 0);
    this->current += index;
    this->centerOn(QPoint(this->rect().width()*1.5, 0));
}


ProgramView::ProgramView(Application application)
{
    this->hide();
    this->icon = 0;
    this->label = 0;
    this->block = 0;
    this->application = application;
}

ProgramView::~ProgramView()
{
    delete this->icon;
    delete this->label;
    delete this->block;
}

void ProgramView::centerItems()
{
    this->icon->setPos(0, 0);
    QRectF iconRect = this->icon->boundingRect();
    QRectF labelRect = this->label->boundingRect();
    QRectF blockRect = this->block->boundingRect();
    this->block->setPos(
        qreal(iconRect.width() / 2 - blockRect.width() / 2),
        qreal(iconRect.height() / 2 - blockRect.height() / 2)
    );
    this->label->setPos(
        qreal(iconRect.width() / 2 - labelRect.width() / 2),
        qreal(iconRect.height() / 2 - labelRect.height() / 2)
    );
    this->setTransformOriginPoint(iconRect.center());
}

void ProgramView::show()
{
    if (this->icon == 0)
        this->icon = new QGraphicsPixmapItem(KIcon(application.icon).pixmap(128), this);
    if (this->label == 0)
        this->label = new QGraphicsTextItem(application.name, this);
    if (this->block == 0)
        this->block = new QGraphicsRectItem(this->label->boundingRect(), this);
    this->block->setBrush(QBrush(Qt::SolidPattern));
    this->block->setOpacity(0.7);
    this->label->setZValue(10);
    this->centerItems();
    QGraphicsItemGroup::show();
}


AppList::AppList()
{}

AppList::~AppList()
{}

void AppList::insertSorted(Application value)
{
    int index = this->length() / 2;
    if (this->length() > 0)
    {
        int span = 1 + this->length() / 2;
        int priority = value.priority;
        int item = this->value(index).priority;
        while (!(
                    priority > this->value(index - 1).priority and
                    priority <= item
                ))
        {
            span -= span / 2;
            if (priority > item)
                index += span;
            else if (priority <= item)
                index -= span;
            if (index < 0)
            {
                index = 0;
                break;
            }
            if (index >= this->length())
            {
                index = this->length();
                break;
            }
            item = this->value(index).priority;
        }
    }
    this->insert(index, value);
}


#include "Mangonel.moc"
// kate: indent-mode cstyle; space-indent on; indent-width 4; 