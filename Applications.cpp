#include "Applications.h"

#include <QDBusInterface>
#include <QSettings>
#include <KDE/KIcon>


const QStringList priorityOrder = (QStringList() << "name" << "genericName" << "categories");


Applications::Applications()
{
    this->appTable = new AppTable();
    QSettings* config = new QSettings();
    config->beginGroup("popularity");
    foreach(QString key, config->allKeys())
    {
        QList<QVariant> list = config->value(key).toList();
        key.replace(":", "/");
        this->popList[key].lastUse = list[0].toInt();
        this->popList[key].count = list[1].toInt();
    }
    delete config;
}

Applications::~Applications()
{}

AppList Applications::getResults(QString query)
{
    AppList list = AppList();
    int count = 0;
    int time = QDateTime().currentMSecsSinceEpoch() / 1000;
    foreach(QString name, this->appTable->keys())
    {
        Plasma::DataEngine::Data item = this->appTable->value(name);
        bool addToList = false;
        int priority = 0;
        if (this->popList.contains(item["entryPath"].toString()))
        {
            priority = time - this->popList[item["entryPath"].toString()].lastUse;
            priority -= 3600 * this->popList[item["entryPath"].toString()].count;
        }
        else
            priority = time;
        foreach(QString key, priorityOrder)
        {
            if (item.contains(key))
            {
                QString value = item[key].toString();
                if (value.contains(query, Qt::CaseInsensitive))
                {
                    addToList = true;
                    if (key.contains("name", Qt::CaseInsensitive) and
                            value.startsWith(query, Qt::CaseInsensitive))
                        priority -= 3600 * 60;
                    priority -= 60 * priorityOrder.indexOf(key);
                }
            }
        }
        if (addToList)
        {
            Application app = Application();
            app.name = item["name"].toString();
            app.icon = item["iconName"].toString();
            app.priority = priority;
            app.object = this;
            app.program = item;
            app.completion = app.name;

            list.append(app);
            count ++;
        }
    }
    return list;
}

void Applications::launch(QVariant selected)
{
    QString entryPath = selected.toHash()["entryPath"].toString();
    // Connect to the KLauncher DBus inteface.
    QDBusInterface* dbus = new QDBusInterface(
        "org.kde.klauncher",
        "/KLauncher",
        "org.kde.KLauncher"
    );
    dbus->call(
        "start_service_by_desktop_path",
        entryPath,
        QStringList(),
        QStringList(),
        "",
        true);
    delete dbus;
    int time = QDateTime().currentMSecsSinceEpoch() / 1000;
    popularity pop = popularity();
    if (this->popList.contains(entryPath))
        pop = this->popList[entryPath];
    pop.lastUse = time;
    pop.count ++;
    this->popList[entryPath] = pop;
    this->storePopularity(entryPath, pop);
}

void Applications::setPopularity(QString entry, popularity pop)
{
    this->popList[entry] = pop;
}

QHash<QString, popularity> Applications::getPopList()
{
    return this->popList;
}

void Applications::storePopularity(QString entryPath, popularity pop)
{
    QSettings* config = new QSettings();
    config->beginGroup("popularity");
    QList<QVariant> list = QList<QVariant>();
    list.append(pop.lastUse);
    list.append(pop.count);
    entryPath.replace("/", ":");
    config->setValue(entryPath, QVariant(list));

    delete config;
}


AppTable::AppTable()
{
    Plasma::DataEngine* dataEngine = Plasma::Applet::dataEngine("apps");
    dataEngine->connectAllSources(this);
}

AppTable::~AppTable()
{}

void AppTable::dataUpdated(QString appName, Plasma::DataEngine::Data data)
{
    if (data["isApp"].toBool() == true and data["categories"].toString() != "Screensaver")
        this->insert(appName, data);
    else
        Plasma::Applet::dataEngine("apps")->disconnectSource(appName, this);
}


#include "Applications.moc"
// kate: indent-mode cstyle; space-indent on; indent-width 4; 