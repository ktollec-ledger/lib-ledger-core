/*
 *
 * CosmosLikeTransactionBuilder
 *
 * Created by El Khalil Bellakrid on  14/06/2019.
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ledger
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

#include <cereal/external/base64.hpp>

#include <rapidjson/document.h>
#include <rapidjson/rapidjson.h>
#include <utils/DateUtils.hpp>
#include <wallet/common/Amount.h>
#include <wallet/cosmos/CosmosLikeConstants.hpp>
#include <wallet/cosmos/CosmosLikeCurrencies.hpp>
#include <wallet/cosmos/CosmosLikeMessage.hpp>
#include <wallet/cosmos/api_impl/CosmosLikeTransactionApi.hpp>
#include <wallet/cosmos/transaction_builders/CosmosLikeTransactionBuilder.hpp>

// Windows + Rapidjson incompatibility
// Windows secretly add a preprocessor macro that
// replaces GetObject with GetObjectA or GetObjectW
// This hack is necessary to compile
// Reference : https://github.com/Tencent/rapidjson/issues/1448
#if defined(_MSC_VER) || defined(__MINGW32__)
#undef GetObject
#endif

using namespace rapidjson;

namespace ledger {
namespace core {

using namespace cosmos::constants;

using rjObject =
    GenericValue<rapidjson::UTF8<char>, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>>::
        Object;

namespace {
auto getString(const rjObject &object, const char *fieldName)
{
    if (!object.HasMember(fieldName)) {
        throw Exception(
            api::ErrorCode::INVALID_ARGUMENT,
            fmt::format(
                "Error while getting {} from rawTransaction : non existing key", fieldName));
    }
    if (!object[fieldName].IsString()) {
        throw Exception(
            api::ErrorCode::INVALID_ARGUMENT,
            fmt::format(
                "Error while getting string {} from rawTransaction : not a string", fieldName));
    }
    return object[fieldName].GetString();
}

// auto getObject(const rjObject& object, const char *fieldName) {
//     if (!object[fieldName].IsObject()) {
//         throw Exception(api::ErrorCode::INVALID_ARGUMENT, fmt::format("Error while getting object
//         {} from rawTransaction", fieldName));
//     }
//     return object[fieldName].GetObject();
// }

cosmos::MsgSend buildMsgSendFromRawMessage(rjObject const &object)
{
    std::vector<cosmos::Coin> amounts;

    if (!object[kValue][kAmount].IsArray() && !object[kValue][kAmount].IsObject()) {
        return {"", "", amounts};
    }
    else if (object[kValue][kAmount].IsObject()) {
        auto amountObject = object[kValue][kAmount].GetObject();
        amounts.push_back(
            cosmos::Coin{getString(amountObject, kAmount), getString(amountObject, kDenom)});
        return cosmos::MsgSend{getString(object[kValue].GetObject(), kFromAddress),
                               getString(object[kValue].GetObject(), kToAddress),
                               amounts};
    }
    else {
        // We are sure object[kValue][kAmount] is an array here

        // the size of the array of amount should be frequently equal to one
        amounts.reserve(object[kValue][kAmount].GetArray().Size());

        for (auto &amount : object[kValue][kAmount].GetArray()) {
            if (amount.IsObject()) {
                auto amountObject = amount.GetObject();
                amounts.push_back(cosmos::Coin{getString(amountObject, kAmount),
                                               getString(amountObject, kDenom)});
            }
        }

        return cosmos::MsgSend{getString(object[kValue].GetObject(), kFromAddress),
                               getString(object[kValue].GetObject(), kToAddress),
                               amounts};
    }
}

cosmos::MsgDelegate buildMsgDelegateFromRawMessage(rjObject const &object)
{
    auto valueObject = object[kValue].GetObject();
    auto amountObject = valueObject[kAmount].GetObject();

    return cosmos::MsgDelegate{
        getString(valueObject, kDelegatorAddress),
        getString(valueObject, kValidatorAddress),
        cosmos::Coin(getString(amountObject, kAmount), getString(amountObject, kDenom))};
}

cosmos::MsgUndelegate buildMsgUndelegateFromRawMessage(rjObject const &object)
{
    auto valueObject = object[kValue].GetObject();
    auto amountObject = valueObject[kAmount].GetObject();

    return cosmos::MsgUndelegate{
        getString(valueObject, kDelegatorAddress),
        getString(valueObject, kValidatorAddress),
        cosmos::Coin(getString(amountObject, kAmount), getString(amountObject, kDenom))};
}

cosmos::MsgBeginRedelegate buildMsgBeginRedelegateFromRawMessage(rjObject const &object)
{
    auto valueObject = object[kValue].GetObject();
    auto amountObject = valueObject[kAmount].GetObject();

    return cosmos::MsgBeginRedelegate{
        getString(valueObject, kDelegatorAddress),
        getString(valueObject, kValidatorSrcAddress),
        getString(valueObject, kValidatorDstAddress),
        cosmos::Coin(getString(amountObject, kAmount), getString(amountObject, kDenom))};
}

cosmos::MsgSubmitProposal buildMsgSubmitProposalFromRawMessage(rjObject const &object)
{
    auto valueObject = object[kValue].GetObject();
    auto contentObject = valueObject[kContent].GetObject();
    auto initialDepositArray = valueObject[kInitialDeposit].GetArray();

    auto content = api::CosmosLikeContent(
        getString(contentObject, kType),
        getString(contentObject, kTitle),
        getString(contentObject, kDescription));

    std::vector<cosmos::Coin> amounts;
    // the size of the array of amounts should be frequently equal to one
    amounts.reserve(initialDepositArray.Size());
    for (auto &amount : initialDepositArray) {
        auto amountObject = amount.GetObject();
        amounts.push_back(
            cosmos::Coin(getString(amountObject, kAmount), getString(amountObject, kDenom)));
    }

    return cosmos::MsgSubmitProposal{content, getString(valueObject, kProposer), amounts};
}

cosmos::MsgVote buildMsgVoteFromRawMessage(rjObject const &object)
{
    auto valueObject = object[kValue].GetObject();
    return cosmos::MsgVote{getString(valueObject, kVoter),
                           getString(valueObject, kProposalId),
                           cosmos::stringToVoteOption(getString(valueObject, kOption))};
}

cosmos::MsgDeposit buildMsgDepositFromRawMessage(rjObject const &object)
{
    auto valueObject = object[kValue].GetObject();
    std::vector<cosmos::Coin> amounts;

    if (valueObject[kAmount].IsArray()) {
        // the size of the array of amount should be frequently equals to one
        amounts.reserve(valueObject[kAmount].GetArray().Size());

        for (auto &amount : valueObject[kAmount].GetArray()) {
            if (amount.IsObject()) {
                auto amountObject = amount.GetObject();

                amounts.push_back(cosmos::Coin{getString(amountObject, kAmount),
                                               getString(amountObject, kDenom)});
            }
        }
    }

    return cosmos::MsgDeposit{
        getString(valueObject, kDepositor), getString(valueObject, kProposalId), amounts};
}

cosmos::MsgWithdrawDelegationReward buildMsgWithdrawDelegationRewardFromRawMessage(
    rjObject const &object)
{
    auto valueObject = object[kValue].GetObject();
    return cosmos::MsgWithdrawDelegationReward{getString(valueObject, kDelegatorAddress),
                                               getString(valueObject, kValidatorAddress)};
}

cosmos::MsgMultiSend buildMsgMultiSendFromRawMessage(rjObject const &object)
{
    auto valueObject = object[kValue].GetObject();
    std::vector<cosmos::MultiSendInput> inputs;
    std::vector<cosmos::MultiSendOutput> outputs;

    if (valueObject.HasMember(kInputs) && valueObject[kInputs].IsArray()) {
        auto inputJson = valueObject[kInputs].GetArray();
        inputs.reserve(inputJson.Size());
        for (auto &input : inputJson) {
            std::vector<cosmos::Coin> inputAmounts;
            if (input.IsObject()) {
                auto singleInput = input.GetObject();
                auto coins = singleInput[kCoins].GetArray();

                inputAmounts.reserve(coins.Size());

                for (auto &amount : coins) {
                    if (amount.IsObject()) {
                        auto amountObject = amount.GetObject();

                        inputAmounts.push_back(cosmos::Coin{getString(amountObject, kAmount),
                                                            getString(amountObject, kDenom)});
                    }
                }

                inputs.push_back({getString(singleInput, kAddress), inputAmounts});
            }
        }
    }

    if (valueObject.HasMember(kOutputs) && valueObject[kOutputs].IsArray()) {
        auto outputJson = valueObject[kOutputs].GetArray();
        outputs.reserve(outputJson.Size());
        for (auto &output : outputJson) {
            std::vector<cosmos::Coin> outputAmounts;
            if (output.IsObject()) {
                auto singleOutput = output.GetObject();
                auto coins = singleOutput[kCoins].GetArray();

                outputAmounts.reserve(coins.Size());

                for (auto &amount : coins) {
                    if (amount.IsObject()) {
                        auto amountObject = amount.GetObject();

                        outputAmounts.push_back(cosmos::Coin{getString(amountObject, kAmount),
                                                             getString(amountObject, kDenom)});
                    }
                }

                outputs.push_back({getString(singleOutput, kAddress), outputAmounts});
            }
        }
    }

    return cosmos::MsgMultiSend{inputs, outputs};
}

cosmos::MsgCreateValidator buildMsgCreateValidatorFromRawMessage(rjObject const &object)
{
    auto valueObject = object[kValue].GetObject();
    cosmos::ValidatorDescription description;
    cosmos::ValidatorCommission commission;
    std::string minSelfDelegation("");
    std::string delegatorAddress("");
    std::string validatorAddress("");
    std::string pubkey("");
    cosmos::Coin value;
    if (valueObject.HasMember(kDescription) && valueObject[kDescription].IsObject()) {
        auto descriptionObject = valueObject[kDescription].GetObject();
        description.moniker = getString(descriptionObject, kMoniker);
        if (descriptionObject.HasMember(kIdentity) && descriptionObject[kIdentity].IsString()) {
            description.identity = optional<std::string>(getString(descriptionObject, kIdentity));
        }
        if (descriptionObject.HasMember(kWebsite) && descriptionObject[kWebsite].IsString()) {
            description.website = optional<std::string>(getString(descriptionObject, kWebsite));
        }
        if (descriptionObject.HasMember(kDetails) && descriptionObject[kDetails].IsString()) {
            description.details = optional<std::string>(getString(descriptionObject, kDetails));
        }
    }

    if (valueObject.HasMember(kCommission) && valueObject[kCommission].IsObject()) {
        auto commissionObject = valueObject[kCommission].GetObject();
        commission.rates.rate = getString(commissionObject, kCommissionRate);
        commission.rates.maxRate = getString(commissionObject, kCommissionMaxRate);
        commission.rates.maxChangeRate = getString(commissionObject, kCommissionMaxChangeRate);
        commission.updateTime = DateUtils::fromJSON(getString(commissionObject, kUpdateTime));
    }

    if (valueObject.HasMember(kValue) && valueObject[kValue].IsObject()) {
        value = cosmos::Coin{
            getString(valueObject[kValue].GetObject(), kAmount),
            getString(valueObject[kValue].GetObject(), kDenom),
        };
    }

    minSelfDelegation = getString(valueObject, kMinSelfDelegation);
    delegatorAddress = getString(valueObject, kDelegatorAddress);
    validatorAddress = getString(valueObject, kValidatorAddress);
    pubkey = getString(valueObject, kPubKey);

    return cosmos::MsgCreateValidator{description,
                                      commission,
                                      minSelfDelegation,
                                      delegatorAddress,
                                      validatorAddress,
                                      pubkey,
                                      value};
}

cosmos::MsgEditValidator buildMsgEditValidatorFromRawMessage(rjObject const &object)
{
    auto valueObject = object[kValue].GetObject();
    optional<cosmos::ValidatorDescription> description;
    std::string validatorAddress("");
    optional<std::string> commissionRate;
    optional<std::string> minSelfDelegation;
    if (valueObject.HasMember(kDescription) && valueObject[kDescription].IsObject()) {
        auto descriptionObject = valueObject[kDescription].GetObject();
        description = cosmos::ValidatorDescription();
        description->moniker = getString(descriptionObject, kMoniker);
        if (descriptionObject.HasMember(kIdentity) && descriptionObject[kIdentity].IsString()) {
            description->identity = optional<std::string>(getString(descriptionObject, kIdentity));
        }
        if (descriptionObject.HasMember(kWebsite) && descriptionObject[kWebsite].IsString()) {
            description->website = optional<std::string>(getString(descriptionObject, kWebsite));
        }
        if (descriptionObject.HasMember(kDetails) && descriptionObject[kDetails].IsString()) {
            description->details = optional<std::string>(getString(descriptionObject, kDetails));
        }
    }

    validatorAddress = getString(valueObject, kValidatorAddress);
    if (valueObject.HasMember(kEditValCommissionRate) &&
        valueObject[kEditValCommissionRate].IsString()) {
        commissionRate = optional<std::string>(getString(valueObject, kEditValCommissionRate));
    }
    if (valueObject.HasMember(kMinSelfDelegation) && valueObject[kMinSelfDelegation].IsString()) {
        minSelfDelegation = optional<std::string>(getString(valueObject, kMinSelfDelegation));
    }

    return cosmos::MsgEditValidator{
        description, validatorAddress, commissionRate, minSelfDelegation};
}

cosmos::MsgSetWithdrawAddress buildMsgSetWithdrawAddressFromRawMessage(rjObject const &object)
{
    auto valueObject = object[kValue].GetObject();
    return cosmos::MsgSetWithdrawAddress{getString(valueObject, kDelegatorAddress),
                                         getString(valueObject, kWithdrawAddress)};
}

cosmos::MsgWithdrawDelegatorReward buildMsgWithdrawDelegatorRewardFromRawMessage(
    rjObject const &object)
{
    auto valueObject = object[kValue].GetObject();
    return cosmos::MsgWithdrawDelegatorReward{getString(valueObject, kDelegatorAddress),
                                              getString(valueObject, kValidatorAddress)};
}

cosmos::MsgWithdrawValidatorCommission buildMsgWithdrawValidatorCommissionFromRawMessage(
    rjObject const &object)
{
    auto valueObject = object[kValue].GetObject();
    return cosmos::MsgWithdrawValidatorCommission{getString(valueObject, kValidatorAddress)};
}

cosmos::MsgUnjail buildMsgUnjailFromRawMessage(rjObject const &object)
{
    auto valueObject = object[kValue].GetObject();
    return cosmos::MsgUnjail{getString(valueObject, kValidatorAddress)};
}
}  // namespace

CosmosLikeTransactionBuilder::CosmosLikeTransactionBuilder(
    const std::shared_ptr<api::ExecutionContext> &context,
    const CosmosLikeTransactionBuildFunction &buildFunction)
{
    _context = context;
    _buildFunction = buildFunction;
}

CosmosLikeTransactionBuilder::CosmosLikeTransactionBuilder(const CosmosLikeTransactionBuilder &cpy)
{
    _buildFunction = cpy._buildFunction;
    _request = cpy._request;
    _context = cpy._context;
}

std::shared_ptr<api::CosmosLikeTransactionBuilder> CosmosLikeTransactionBuilder::setSequence(
    const std::string &sequence)
{
    _request.sequence = sequence;
    return shared_from_this();
}

std::shared_ptr<api::CosmosLikeTransactionBuilder> CosmosLikeTransactionBuilder::setAccountNumber(
    const std::string &accountNumber)
{
    _request.accountNumber = accountNumber;
    return shared_from_this();
}

std::shared_ptr<api::CosmosLikeTransactionBuilder> CosmosLikeTransactionBuilder::setMemo(
    const std::string &memo)
{
    _request.memo = memo;
    return shared_from_this();
}

std::shared_ptr<api::CosmosLikeTransactionBuilder> CosmosLikeTransactionBuilder::setGas(
    const std::shared_ptr<api::Amount> &gas)
{
    _request.gas = std::make_shared<BigInt>(gas->toString());
    return shared_from_this();
}

std::shared_ptr<api::CosmosLikeTransactionBuilder> CosmosLikeTransactionBuilder::setGasAdjustment(
    double gasAdjusment)
{
    _request.gasAdjustment = gasAdjusment;
    return shared_from_this();
}

std::shared_ptr<api::CosmosLikeTransactionBuilder> CosmosLikeTransactionBuilder::setFee(
    const std::shared_ptr<api::Amount> &fee)
{
    _request.fee = std::make_shared<BigInt>(fee->toString());
    return shared_from_this();
}

std::shared_ptr<api::CosmosLikeTransactionBuilder> CosmosLikeTransactionBuilder::setCorrelationId(
    const std::string &correlationId) 
{
    _request.correlationId = correlationId;
    return shared_from_this();
}

std::shared_ptr<api::CosmosLikeTransactionBuilder> CosmosLikeTransactionBuilder::addMessage(
    const std::shared_ptr<api::CosmosLikeMessage> &message)
{
    _request.messages.push_back(message);
    return shared_from_this();
}

void CosmosLikeTransactionBuilder::build(
    const std::shared_ptr<api::CosmosLikeTransactionCallback> &callback)
{
    build().callback(_context, callback);
}

Future<std::shared_ptr<api::CosmosLikeTransaction>> CosmosLikeTransactionBuilder::build()
{
    return _buildFunction(_request);
}

std::shared_ptr<api::CosmosLikeTransactionBuilder> CosmosLikeTransactionBuilder::clone()
{
    return std::make_shared<CosmosLikeTransactionBuilder>(*this);
}

void CosmosLikeTransactionBuilder::reset()
{
    _request = CosmosLikeTransactionBuildRequest();
}

std::shared_ptr<api::CosmosLikeTransaction>
api::CosmosLikeTransactionBuilder::parseRawUnsignedTransaction(
    const api::Currency &currency, const std::string &rawTransaction)
{
    return ::ledger::core::CosmosLikeTransactionBuilder::parseRawTransaction(
        currency, rawTransaction, false);
}

std::shared_ptr<api::CosmosLikeTransaction>
api::CosmosLikeTransactionBuilder::parseRawSignedTransaction(
    const api::Currency &currency, const std::string &rawTransaction)
{
    return ::ledger::core::CosmosLikeTransactionBuilder::parseRawTransaction(
        currency, rawTransaction, true);
}

// Parse a raw transaction, formatted either
// to be signed by the device :
// https://github.com/cosmos/ledger-cosmos-app/blob/master/docs/TXSPEC.md#format or to be broadcast
// to the network :
// https://github.com/cosmos/cosmos-sdk/blob/2e42f9cb745aaa4c1a52ee730a969a5eaa938360/x/auth/types/stdtx.go#L23-L30
std::shared_ptr<api::CosmosLikeTransaction> CosmosLikeTransactionBuilder::parseRawTransaction(
    const api::Currency &currency, const std::string &rawTransaction, bool isSigned)
{
    Document document;
    document.Parse(rawTransaction.c_str());

    auto tx = std::make_shared<CosmosLikeTransactionApi>();

    tx->setCurrency(currency);
    tx->setMemo(getString(document.GetObject(), kMemo));

    // Raw tx has similar format for broadcast and for signature,
    // but with some differences: e.g "account_number" and "sequence"
    // are present for signature but not for broadcast
    if (document.HasMember(kAccountNumber)) {
        tx->setAccountNumber(getString(document.GetObject(), kAccountNumber));
    }
    if (document.HasMember(kSequence)) {
        tx->setSequence(getString(document.GetObject(), kSequence));
    }

    // Fees
    if (document[kFee].IsObject()) {
        auto feeObject = document[kFee].GetObject();

        // Gas Limit
        auto gas = std::make_shared<BigInt>(getString(feeObject, kGas));
        tx->setGas(gas);

        // Total Tx fees
        // Gas Price is then deduced with Total_Tx_fees / Gas Limit
        if (feeObject[kAmount].IsArray()) {
            auto fee = BigInt();

            auto getAmount = [=](const rjObject &object) -> Amount {
                auto denom = getString(object, kDenom);
                auto amount = getString(object, kAmount);

                auto unit = std::find_if(
                    currency.units.begin(),
                    currency.units.end(),
                    [&](const api::CurrencyUnit &unit) {
                        return unit.name == denom;
                    });

                // NOTE until other units are wanted, we assume only uatom is used.
                // This will only fail in debug builds.
                assert(unit->name == "uatom");

                if (unit == currency.units.end()) {
                    throw Exception(
                        api::ErrorCode::INVALID_ARGUMENT, "Unknown unit while parsing transaction");
                }
                return Amount(
                    currency,
                    0,
                    BigInt(amount) *
                        BigInt(10).pow(static_cast<unsigned short>((*unit).numberOfDecimal)));
            };

            // accumlate all types of fee
            for (auto &amount : feeObject[kAmount].GetArray()) {
                if (amount.IsObject()) {
                    fee = fee + BigInt(getAmount(amount.GetObject()).toString());
                }
            }

            tx->setFee(std::make_shared<BigInt>(fee));
        }
    }

    // Messages
    // Raw tx has similar format for broadcast and for signature,
    // but with some differences: e.g "msgs" for signature and "msg" for broadcast
    std::string msgKey = "";
    if (document.HasMember(kMessage)) {
        msgKey = kMessage;
    }
    else if (document.HasMember(kMessages)) {
        msgKey = kMessages;
    }

    if (!msgKey.empty()) {
        std::vector<std::shared_ptr<api::CosmosLikeMessage>> messages;

        messages.reserve(document[msgKey.c_str()].GetArray().Size());

        for (auto &msg : document[msgKey.c_str()].GetArray()) {
            if (msg.IsObject()) {
                auto msgObject = msg.GetObject();

                switch (cosmos::stringToMsgType(getString(msgObject, kType))) {
                case api::CosmosLikeMsgType::MSGSEND:
                    messages.push_back(
                        api::CosmosLikeMessage::wrapMsgSend(buildMsgSendFromRawMessage(msgObject)));
                    break;
                case api::CosmosLikeMsgType::MSGDELEGATE:
                    messages.push_back(api::CosmosLikeMessage::wrapMsgDelegate(
                        buildMsgDelegateFromRawMessage(msgObject)));
                    break;
                case api::CosmosLikeMsgType::MSGUNDELEGATE:
                    messages.push_back(api::CosmosLikeMessage::wrapMsgUndelegate(
                        buildMsgUndelegateFromRawMessage(msgObject)));
                    break;
                case api::CosmosLikeMsgType::MSGBEGINREDELEGATE:
                    messages.push_back(api::CosmosLikeMessage::wrapMsgBeginRedelegate(
                        buildMsgBeginRedelegateFromRawMessage(msgObject)));
                    break;
                case api::CosmosLikeMsgType::MSGSUBMITPROPOSAL:
                    messages.push_back(api::CosmosLikeMessage::wrapMsgSubmitProposal(
                        buildMsgSubmitProposalFromRawMessage(msgObject)));
                    break;
                case api::CosmosLikeMsgType::MSGVOTE:
                    messages.push_back(
                        api::CosmosLikeMessage::wrapMsgVote(buildMsgVoteFromRawMessage(msgObject)));
                    break;
                case api::CosmosLikeMsgType::MSGDEPOSIT:
                    messages.push_back(api::CosmosLikeMessage::wrapMsgDeposit(
                        buildMsgDepositFromRawMessage(msgObject)));
                    break;
                case api::CosmosLikeMsgType::MSGWITHDRAWDELEGATIONREWARD:
                    messages.push_back(api::CosmosLikeMessage::wrapMsgWithdrawDelegationReward(
                        buildMsgWithdrawDelegationRewardFromRawMessage(msgObject)));
                    break;
                case api::CosmosLikeMsgType::MSGMULTISEND:
                    messages.push_back(api::CosmosLikeMessage::wrapMsgMultiSend(
                        buildMsgMultiSendFromRawMessage(msgObject)));
                    break;
                case api::CosmosLikeMsgType::MSGCREATEVALIDATOR:
                    messages.push_back(api::CosmosLikeMessage::wrapMsgCreateValidator(
                        buildMsgCreateValidatorFromRawMessage(msgObject)));
                    break;
                case api::CosmosLikeMsgType::MSGEDITVALIDATOR:
                    messages.push_back(api::CosmosLikeMessage::wrapMsgEditValidator(
                        buildMsgEditValidatorFromRawMessage(msgObject)));
                    break;
                case api::CosmosLikeMsgType::MSGSETWITHDRAWADDRESS:
                    messages.push_back(api::CosmosLikeMessage::wrapMsgSetWithdrawAddress(
                        buildMsgSetWithdrawAddressFromRawMessage(msgObject)));
                    break;
                case api::CosmosLikeMsgType::MSGWITHDRAWDELEGATORREWARD:
                    messages.push_back(api::CosmosLikeMessage::wrapMsgWithdrawDelegatorReward(
                        buildMsgWithdrawDelegatorRewardFromRawMessage(msgObject)));
                    break;
                case api::CosmosLikeMsgType::MSGWITHDRAWVALIDATORCOMMISSION:
                    messages.push_back(api::CosmosLikeMessage::wrapMsgWithdrawValidatorCommission(
                        buildMsgWithdrawValidatorCommissionFromRawMessage(msgObject)));
                    break;
                case api::CosmosLikeMsgType::MSGUNJAIL:
                    messages.push_back(api::CosmosLikeMessage::wrapMsgUnjail(
                        buildMsgUnjailFromRawMessage(msgObject)));
                    break;
                default: {
                    cosmos::Message msg;
                    msg.type = getString(msgObject, kType);
                    msg.content = cosmos::MsgUnsupported();
                    messages.push_back(std::make_shared<::ledger::core::CosmosLikeMessage>(msg));
                } break;
                }
            }
        }

        tx->setMessages(messages);
    }

    // Signatures (only in broadcast format)
    if (isSigned && document.HasMember(kSignatures) && document[kSignatures].IsArray() &&
        !document[kSignatures].GetArray().Empty()) {
        auto firstSignature = document[kSignatures].GetArray()[0].GetObject();

        {  // Signature
            std::string sig = getString(firstSignature, kSignature);
            auto decoded = cereal::base64::decode(sig);
            std::vector<uint8_t> rSig(decoded.begin(), decoded.begin() + 32);
            std::vector<uint8_t> sSig(decoded.begin() + 32, decoded.begin() + 64);
            tx->setSignature(rSig, sSig);
        }

        {  // Optional public key
            if (firstSignature.HasMember(kPubKey) &&
                firstSignature[kPubKey].GetObject().HasMember(kValue)) {
                std::string pubKey = firstSignature[kPubKey].GetObject()[kValue].GetString();
                auto decoded = cereal::base64::decode(pubKey);
                std::vector<uint8_t> publicKey(decoded.begin(), decoded.end());
                tx->setSigningPubKey(publicKey);
            }
        }
    }

    return tx;
}
}  // namespace core
}  // namespace ledger
