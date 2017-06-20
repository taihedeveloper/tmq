/*
 * consumer.cpp
 *
 *  Created on: Mar 15, 2017
 *      Author: root
 */

#include "consumer.h"
#include "common.h"


Consumer::Consumer():
_connected(false)
{
    // TODO Auto-generated constructor stub

}

Consumer::~Consumer()
{
    // TODO Auto-generated destructor stub
}

bool Consumer::IsConnected()
{
    return _connected;
}

