#pragma once
#include "interfaces/CryptoProvider.hpp"
namespace baz
{
void VerifyThreshold(const CryptoProvider &crypto, const Json &envelope, const Json &keys, const Json &role);
}
