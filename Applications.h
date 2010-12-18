#ifndef Applications_H
#define Applications_H

#include "Provider.h"

#include <KDE/Plasma/Applet>
#include <KDE/Plasma/DataEngine>

struct popularity
{
    int lastUse;
    int count;
};

class AppTable : Plasma::Applet, public QHash<QString, Plasma::DataEngine::Data>
{
    Q_OBJECT
public:
    AppTable();
    ~AppTable();
private slots:
    void dataUpdated(QString appName, Plasma::DataEngine::Data data);
};

class Applications : public Provider
{
    Q_OBJECT
public:
    Applications();
    ~Applications();
private:
    void setPopularity(QString entry, popularity pop);
    QHash<QString, popularity> getPopList();
    void storePopularity(QString entryPath, popularity pop);
    AppTable* appTable;
    QHash<QString, popularity> popList;
public slots:
    AppList getResults(QString query);
    void launch(QVariant selected);
};


#endif //Applications_H
// kate: indent-mode cstyle; space-indent on; indent-width 4; 
