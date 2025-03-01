/*
 * This file is part of QBDI.
 *
 * Copyright 2017 - 2024 Quarkslab
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <catch2/catch.hpp>
#include "API/OptionsTest.h"

#include <algorithm>
#include <sstream>
#include <string>
#include "inttypes.h"

#include "QBDI/Memory.hpp"
#include "QBDI/Platform.h"

TEST_CASE_METHOD(OptionsTest, "OptionsTest_X86_64-ATTSyntax") {

  InMemoryObject leaObj("leaq (%rax), %rbx\nret\n");
  QBDI::rword addr = (QBDI::rword)leaObj.getCode().data();

  vm.setOptions(QBDI::Options::NO_OPT);
  vm.precacheBasicBlock(addr);

  const QBDI::InstAnalysis *ana =
      vm.getCachedInstAnalysis(addr, QBDI::ANALYSIS_DISASSEMBLY);
  REQUIRE(ana != nullptr);
  REQUIRE(ana->disassembly != nullptr);

  CHECK(std::string(ana->disassembly) == std::string("\tlea\trbx, [rax]"));

  vm.clearAllCache();
  vm.setOptions(QBDI::Options::OPT_ATT_SYNTAX);
  vm.precacheBasicBlock(addr);

  ana = vm.getCachedInstAnalysis(addr, QBDI::ANALYSIS_DISASSEMBLY);
  REQUIRE(ana != nullptr);
  REQUIRE(ana->disassembly != nullptr);

  CHECK(std::string(ana->disassembly) == std::string("\tleaq\t(%rax), %rbx"));
}

static QBDI::VMAction setBool(QBDI::VMInstanceRef vm, QBDI::GPRState *gprState,
                              QBDI::FPRState *fprState, void *data) {
  *((bool *)data) = true;
  return QBDI::VMAction::CONTINUE;
}

TEST_CASE_METHOD(OptionsTest, "OptionsTest_X86_64-setOption") {
  // check if the callback and the instrumentation range are keep when use
  // setOptions

  InMemoryObject leaObj("leaq 0x20(%rax), %rax\nret\n");
  QBDI::rword addr = (QBDI::rword)leaObj.getCode().data();

  uint8_t *fakestack;
  QBDI::GPRState *state = vm.getGPRState();
  bool ret = QBDI::allocateVirtualStack(state, 4096, &fakestack);
  REQUIRE(ret == true);
  state->rax = 0;

  vm.setOptions(QBDI::Options::NO_OPT);
  vm.addInstrumentedRange(addr, addr + (QBDI::rword)leaObj.getCode().size());

  bool cbReach = false;
  vm.addCodeCB(QBDI::POSTINST, setBool, &cbReach);

  QBDI::rword retval;

  REQUIRE(vm.call(&retval, addr, {}));
  REQUIRE(cbReach);
  REQUIRE(state->rax == 0x20);

  cbReach = false;
  vm.setOptions(QBDI::Options::OPT_ATT_SYNTAX);
  REQUIRE(vm.call(&retval, addr, {}));
  REQUIRE(cbReach);
  REQUIRE(state->rax == 0x40);

  cbReach = false;
  vm.setOptions(QBDI::Options::OPT_DISABLE_FPR);
  REQUIRE(vm.call(&retval, addr, {}));
  REQUIRE(cbReach);
  REQUIRE(state->rax == 0x60);

  cbReach = false;
  vm.setOptions(QBDI::Options::OPT_DISABLE_OPTIONAL_FPR);
  REQUIRE(vm.call(&retval, addr, {}));
  REQUIRE(cbReach);
  REQUIRE(state->rax == 0x80);

  QBDI::alignedFree(fakestack);
}
