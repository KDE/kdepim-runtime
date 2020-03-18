/*
   Copyright (C) 2019 Daniel Vrátil <dvratil@kde.org>

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef KGAPIVERSIONATTRIBUTE
#define KGAPIVERSIONATTRIBUTE

#include <AkonadiCore/Attribute>

class KGAPIVersionAttribute : public Akonadi::Attribute
{
public:
    explicit KGAPIVersionAttribute() = default;
    explicit KGAPIVersionAttribute(uint32_t version);
    ~KGAPIVersionAttribute() override = default;

    uint32_t version() const
    {
        return mVersion;
    }

    void setVersion(uint32_t version)
    {
        mVersion = version;
    }

    QByteArray type() const override;
    Akonadi::Attribute *clone() const override;
    QByteArray serialized() const override;
    void deserialize(const QByteArray &data) override;

private:
    uint32_t mVersion = 0;
};

#endif
