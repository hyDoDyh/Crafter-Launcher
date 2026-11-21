#ifndef MINECTAFTMODEL_H
#define MINECTAFTMODEL_H
#include <QAbstractListModel>
#include <QList>
#include "minecraftversion.h"
class MinectaftModel : public QAbstractListModel
{
    Q_OBJECT
public:
    MinectaftModel();
    enum Roles {
        IdRole = Qt::UserRole + 1,
        TypeRole,
        TimeRole
    };
    int rowCount(const QModelIndex &parent = QModelIndex()) const override {
        return mcs.size();
    }
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override {
        if(!index.isValid())return QVariant();

        MinecraftVersion version = mcs.at(index.row());
        switch (role) {
        case Qt::DisplayRole:
        case IdRole:
            return version.getId();
        case TypeRole:
            return version.getType();
        case TimeRole:
            return version.getReleaseTime();
        default:
            return QVariant();
        }
    }
    void AddVersion(const MinecraftVersion &ver) {
        beginInsertRows(QModelIndex(), mcs.size(), mcs.size());
        mcs.append(ver);
        endInsertRows();
    }
private:
    QList<MinecraftVersion> mcs ;
};

#endif // MINECTAFTMODEL_H
