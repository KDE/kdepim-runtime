#include "conversion.h"

Akonadi::Collection::List intoAkonadi(const rust::Vec<Collection> &cols)
{
    Akonadi::Collection::List list;
    list.reserve(cols.size());
    std::transform(cols.cbegin(), cols.cend(), std::back_inserter(list), [](const auto &inCol) {
        Akonadi::Collection col;
        col.setId(inCol.id);
        col.setName(QString::fromStdString(static_cast<std::string>(inCol.name)));
        col.setRemoteId(QString::fromStdString(static_cast<std::string>(inCol.remoteId)));
        col.setRemoteRevision(QString::fromStdString(static_cast<std::string>(inCol.remoteRevision)));
        return col;
    });
    return list;
}
