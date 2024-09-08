#pragma once

#include "cxx-qt-gen/resource_state.cxxqt.h"
#include <Akonadi/Collection>

Akonadi::Collection::List intoAkonadi(const rust::Vec<Collection> &cols);
