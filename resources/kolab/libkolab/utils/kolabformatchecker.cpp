/*
 * Copyright (C) 2012  Sofia Balicka <balicka@kolabsys.com>
 * Copyright (C) 2013  Christian Mollekopf <mollekopf@kolabsys.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <boost/program_options.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <QString>
#include <QFile>
#include <kolabformat.h>
#include "kolabformat/errorhandler.h"
#include "kolabformat/kolabobject.h"

namespace po = boost::program_options;
using namespace std;

KMime::Message::Ptr readMimeFile( const QString &fileName, bool &ok)
{
    QFile file( fileName );
    ok = file.open( QFile::ReadOnly );
    if (!ok) {
        cout << "failed to open file: " << fileName.toStdString() << endl;;
        return KMime::Message::Ptr();
    }
    const QByteArray data = file.readAll();
    KMime::Message::Ptr msg = KMime::Message::Ptr(new KMime::Message);
    msg->setContent( KMime::CRLFtoLF(data) );
    msg->parse();
    return msg;
}

int main(int argc, char *argv[])
{
    // Declare the supported options.
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "produce help message")
        ("input-file", po::value<std::vector<std::string> >(), "input files")
        ;

    po::positional_options_description p;
    p.add("input-file", -1);

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).
            options(desc).positional(p).run(), vm);
    po::notify(vm);

    if (vm.count("help")) {
        cout << desc << "\n";
        return 1;
    }

    vector<string> inputFiles;
    if (vm.count("input-file")) {
        inputFiles = vm["input-file"].as< vector<string> >();
    } else {
        cout << "Specify input-file\n";
        return -1;
    }

    int returnValue = 0;

    cout << endl;

    for(vector<string>::const_iterator it = inputFiles.begin();
            it != inputFiles.end(); it++){

        cout << "File: " << *it << endl;

        bool ok;
        KMime::Message::Ptr message = readMimeFile( QString::fromStdString(*it), ok);

        if(!ok){
            returnValue = -1;
            cout << endl;
            continue;
        }

        Kolab::KolabObjectReader reader(message);

        if (Kolab::ErrorHandler::errorOccured()){
            cout << "Errors occured during parsing." << endl;
            returnValue = -1;
        } else {
            cout << "Parsed message without error." << endl;
        }

        cout << endl;
    }

    return returnValue;
}

