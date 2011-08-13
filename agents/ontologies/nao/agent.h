#ifndef _NAO_AGENT_H_
#define _NAO_AGENT_H_

#include <QtCore/QVariant>
#include <QtCore/QStringList>
#include <QtCore/QUrl>
#include <QtCore/QDate>
#include <QtCore/QTime>
#include <QtCore/QDateTime>
#include <Soprano/Vocabulary/RDF>

#include <dms-copy/simpleresource.h>

namespace Nepomuk {
namespace NAO {
/**
 * An agent is the artificial counterpart to nao:Party. It can 
 * be a software component or some service. 
 */
class Agent
{
public:
    Agent(Nepomuk::SimpleResource* res)
      : m_res(res)
    {}

    virtual ~Agent() {}

