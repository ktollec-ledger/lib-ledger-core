/*
 *
 * AccountTests
 * ledger-core-cosmos
 *
 * Created by Gerry Agbobada on 2020/02/12.
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Ledger
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 *
 */

#include <gtest/gtest.h>

#include "../integration/BaseFixture.h"
#include <wallet/cosmos/CosmosLikeWallet.hpp>
#include <wallet/cosmos/CosmosLikeCurrencies.hpp>
#include <wallet/cosmos/factories/CosmosLikeWalletFactory.hpp>

struct CosmosAccounts : public BaseFixture {

};

TEST_F(CosmosAccounts, DISABLED_FirstATOMAccountInfo) {
    auto const currency = currencies::ATOM;
    auto pool = newDefaultPool();
    backend->enableQueryLogging(true);

    auto wallet = uv::wait(pool->createWallet(randomWalletName(), currency.name, api::DynamicObject::newInstance()));
    auto info = uv::wait(wallet->getNextAccountCreationInfo());

    EXPECT_EQ(info.index, 0);
    EXPECT_EQ(info.owners.size(), 1);
    EXPECT_EQ(info.derivations.size(), 1);
    EXPECT_EQ(info.owners[0], "main");
    EXPECT_EQ(info.derivations[0], "44'/118'/0'/0/0");
}
