/*
 *
 * EthereumLikeTransactionBuilder
 *
 * Created by El Khalil Bellakrid on 12/07/2018.
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Ledger
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
 */


#include "EthereumLikeTransactionBuilder.h"
#include <math/BigInt.h>
#include <api/EthereumLikeTransactionCallback.hpp>
#include <bytes/RLP/RLPDecoder.h>
#include <wallet/ethereum/api_impl/EthereumLikeTransactionApi.h>
#include <math/Base58.hpp>

namespace ledger {
    namespace core {

        EthereumLikeTransactionBuilder::EthereumLikeTransactionBuilder(const std::shared_ptr<api::ExecutionContext>& context,
                                                                       const api::Currency& currency,
                                                                       const std::shared_ptr<EthereumLikeBlockchainExplorer> &explorer,
                                                                       const std::shared_ptr<spdlog::logger>& logger,
                                                                       const EthereumLikeTransactionBuildFunction& buildFunction) {
            _context = context;
            _currency = currency;
            _explorer = explorer;
            _build = buildFunction;
            _logger = logger;
            _request.wipe = false;
        }

        EthereumLikeTransactionBuilder::EthereumLikeTransactionBuilder(const EthereumLikeTransactionBuilder& cpy) {
            _currency = cpy._currency;
            _build = cpy._build;
            _request = cpy._request;
            _context = cpy._context;
            _logger = cpy._logger;
        }

        std::shared_ptr<api::EthereumLikeTransactionBuilder>
        EthereumLikeTransactionBuilder::sendToAddress(const std::shared_ptr<api::Amount> & amount,
                                                      const std::string & address) {
            _request.value = std::make_shared<BigInt>(amount->toString());
            _request.toAddress = Base58::encodeWithEIP55(address);
            return shared_from_this();
        }

        std::shared_ptr<api::EthereumLikeTransactionBuilder>
        EthereumLikeTransactionBuilder::wipeToAddress(const std::string & address) {
            _request.toAddress = Base58::encodeWithEIP55(address);
            _request.wipe = true;
            return shared_from_this();
        }

        std::shared_ptr<api::EthereumLikeTransactionBuilder>
        EthereumLikeTransactionBuilder::setGasPrice(const std::shared_ptr<api::Amount> & gasPrice) {
            _request.gasPrice = std::make_shared<BigInt>(gasPrice->toString());
            return shared_from_this();
        }

        std::shared_ptr<api::EthereumLikeTransactionBuilder>
        EthereumLikeTransactionBuilder::setCorrelationId(const std::string & correlationId) {
            _request.correlationId = correlationId;
            return shared_from_this();
        }

        std::shared_ptr<api::EthereumLikeTransactionBuilder>
        EthereumLikeTransactionBuilder::setGasLimit(const std::shared_ptr<api::Amount> & gasLimit) {
            _request.gasLimit = std::make_shared<BigInt>(gasLimit->toString());
            return shared_from_this();
        }

        std::shared_ptr<api::EthereumLikeTransactionBuilder>
        EthereumLikeTransactionBuilder::setInputData(const std::vector<uint8_t> & data) {
            _request.inputData = data;
            return shared_from_this();
        }

        void EthereumLikeTransactionBuilder::build(const std::shared_ptr<api::EthereumLikeTransactionCallback> & callback) {
            build().callback(_context, callback);
        }

        Future<std::shared_ptr<api::EthereumLikeTransaction>> EthereumLikeTransactionBuilder::build() {
            return _build(_request, _explorer);
        }

        std::shared_ptr<api::EthereumLikeTransactionBuilder> EthereumLikeTransactionBuilder::clone() {
            return std::make_shared<EthereumLikeTransactionBuilder>(*this);
        }

        void EthereumLikeTransactionBuilder::reset() {
            _request = EthereumLikeTransactionBuildRequest();
        }

        std::shared_ptr<api::EthereumLikeTransaction>
        api::EthereumLikeTransactionBuilder::parseRawUnsignedTransaction(const api::Currency & currency,
                                                                         const std::vector<uint8_t> & rawTransaction) {
            return ::ledger::core::EthereumLikeTransactionBuilder::parseRawTransaction(currency, rawTransaction, false);
        }

        std::shared_ptr<api::EthereumLikeTransaction>
        api::EthereumLikeTransactionBuilder::parseRawSignedTransaction(const api::Currency & currency,
                                                                       const std::vector<uint8_t> & rawTransaction) {
            return ::ledger::core::EthereumLikeTransactionBuilder::parseRawTransaction(currency, rawTransaction, true);
        }

        std::shared_ptr<api::EthereumLikeTransaction>
        EthereumLikeTransactionBuilder::parseRawTransaction(const api::Currency & currency,
                                                            const std::vector<uint8_t> & rawTransaction,
                                                            bool isSigned) {
            auto decodedRawTx = RLPDecoder::decode(rawTransaction);
            auto children = decodedRawTx->getChildren();

            //TODO: throw if size is KO
            auto tx = std::make_shared<EthereumLikeTransactionApi>(currency);
            int index = 0;
            std::vector<uint8_t> vSignature, rSignature, sSignature;
            for (auto &child: children) {
                if (child->isList()) {
                    throw make_exception(api::ErrorCode::INVALID_ARGUMENT, "No List in this TX");
                }
                auto childHexString = child->toString();
                auto bigIntChild = std::shared_ptr<BigInt>(BigInt::from_hex(childHexString));
                switch (index) {
                    case 0:
                        tx->setNonce(bigIntChild);
                        break;
                    case 1:
                        tx->setGasPrice(bigIntChild);
                        break;
                    case 2:
                        tx->setGasLimit(bigIntChild);
                        break;
                    case 3: {
                        auto ethAddress = "0x" + childHexString;
                        tx->setReceiver(Base58::encodeWithEIP55(ethAddress));
                        break;
                    }
                    case 4:
                        tx->setValue(bigIntChild);
                        break;
                    case 5:
                        tx->setData(hex::toByteArray(childHexString));
                        break;
                    case 6:
                        vSignature = hex::toByteArray(childHexString);
                        break;
                    case 7: //6 would be the 'V' field of V,R and S signature
                        //R signature
                        rSignature = hex::toByteArray(childHexString);
                        break;
                    case 8:
                        //S signature
                        sSignature = hex::toByteArray(childHexString);
                        break;
                    default:
                        break;
                }
                index ++;
            }

            if (isSigned) {
                tx->setSignature(vSignature, rSignature, sSignature);
            }

            return tx;
        }

    }
}