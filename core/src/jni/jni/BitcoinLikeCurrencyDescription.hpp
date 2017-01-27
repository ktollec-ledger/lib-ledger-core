// AUTOGENERATED FILE - DO NOT MODIFY!
// This file generated by Djinni from bitcoin_like_wallet.djinni

#pragma once

#include "../../api/BitcoinLikeCurrencyDescription.hpp"
#include "djinni_support.hpp"

namespace djinni_generated {

class BitcoinLikeCurrencyDescription final {
public:
    using CppType = ::ledger::core::api::BitcoinLikeCurrencyDescription;
    using JniType = jobject;

    using Boxed = BitcoinLikeCurrencyDescription;

    ~BitcoinLikeCurrencyDescription();

    static CppType toCpp(JNIEnv* jniEnv, JniType j);
    static ::djinni::LocalRef<JniType> fromCpp(JNIEnv* jniEnv, const CppType& c);

private:
    BitcoinLikeCurrencyDescription();
    friend ::djinni::JniClass<BitcoinLikeCurrencyDescription>;

    const ::djinni::GlobalRef<jclass> clazz { ::djinni::jniFindClass("co/ledger/core/BitcoinLikeCurrencyDescription") };
    const jmethodID jconstructor { ::djinni::jniGetMethodID(clazz.get(), "<init>", "(I[B[B[BLjava/lang/String;)V") };
    const jfieldID field_BIP44CoinType { ::djinni::jniGetFieldID(clazz.get(), "BIP44CoinType", "I") };
    const jfieldID field_P2PKHAddressVersion { ::djinni::jniGetFieldID(clazz.get(), "P2PKHAddressVersion", "[B") };
    const jfieldID field_P2SHAddressVersion { ::djinni::jniGetFieldID(clazz.get(), "P2SHAddressVersion", "[B") };
    const jfieldID field_XPUBAddressVersion { ::djinni::jniGetFieldID(clazz.get(), "XPUBAddressVersion", "[B") };
    const jfieldID field_shortName { ::djinni::jniGetFieldID(clazz.get(), "shortName", "Ljava/lang/String;") };
};

}  // namespace djinni_generated