/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * File:   utils.h
 * Author: pgj
 *
 * Created on 7 January 2022, 18.20
 */



#ifndef UTILS_H
#define UTILS_H

#include "PetriEngine/Colored/ColoredPetriNetBuilder.h"
#include "VerifyPN.h"

using namespace PetriEngine;
using namespace PetriEngine::Colored;

std::ifstream loadFile(const char* file) {
    std::stringstream ss;
    ss << getenv("TEST_FILES") << file;
    return std::ifstream(ss.str());
}

class ResultHandler : public Reachability::AbstractHandler {

    virtual std::pair<Result, bool> handle(
        size_t index,
        PQL::Condition* query,
        Result result,
        const std::vector<uint32_t>* maxPlaceBound = nullptr,
        size_t expandedStates = 0,
        size_t exploredStates = 0,
        size_t discoveredStates = 0,
        int maxTokens = 0,
        Structures::StateSetInterface* stateset = nullptr, size_t lastmarking = 0, const MarkVal* initialMarking = nullptr, bool = true) {
        if (result == Unknown) return std::make_pair(Unknown, false);
        auto retval = Satisfied;
        if (result == Satisfied)
            retval = query->isInvariant() ? NotSatisfied : Satisfied;
        else if (result == NotSatisfied)
            retval = query->isInvariant() ? Satisfied : NotSatisfied;
        return std::make_pair(retval, false);
    }
};

auto load_pn(std::string model, std::string queries, const std::set<size_t>& qnums)
{

    ColoredPetriNetBuilder cpnBuilder;
    auto f = loadFile(model.c_str());
    cpnBuilder.parse_model(f);
    auto [builder, trans_names, place_names] = unfold(cpnBuilder, false, false, false, std::cerr);
    builder.sort();
    auto q = loadFile(queries.c_str());
    std::vector<std::string> qstrings;
    auto conditions = parseXMLQueries(qstrings, q, qnums, false);
    std::unique_ptr<PetriNet> pn{builder.makePetriNet()};
    contextAnalysis(cpnBuilder, trans_names, place_names, builder, pn.get(), conditions);
    return std::make_tuple(std::move(pn), std::move(conditions), std::move(qstrings));
}

#endif /* UTILS_H */

