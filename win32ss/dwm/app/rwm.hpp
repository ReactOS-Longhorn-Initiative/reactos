#pragma once

#include <rwm.h>
#include "rwmapp.h"
#include <LpcConnectLib.hpp>
#include <LpcCreateLib.hpp>
#include "RWMUserFace.hpp"

VOID
WINAPI
RWMCreateSessionPort();

VOID
WINAPI
RWMConnectToUxServ();
