//
// Copyright (C) 2019 Linkworld Open Team
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https: //www.gnu.org/licenses/>.

#include "./stream.h"
#include "./util/util.h"

namespace moss {

void* CoSendSide(void* arg) {
  auto sendside = reinterpret_cast<SendSide*>(arg);
  while (true) {
    sendside->ConsumeCmd();
    if (sendside->fsm_.Run() != -1) break;
  }
}

SendSide::SendSide() {
  fsm_.When(TriggerType::kStream,
            FSM::transition_t(State::kReady, State::kSend));
  fsm_.When(TriggerType::kStreamDataBlocked,
            FSM::transition_t(State::kReady, State::kSend));
  fsm_.When(TriggerType::kCreateBiStream,
            FSM::transition_t(State::kReady, State::kSend));
  fsm_.When(TriggerType::kStreamFin,
            FSM::transition_t(State::kSend, State::kDataSent));
  fsm_.When(TriggerType::kResetStream,
            FSM::transition_t(State::kReady, State::kResetSent));
  fsm_.When(TriggerType::kResetStream,
            FSM::transition_t(State::kSend, State::kResetSent));
  fsm_.When(TriggerType::kResetStream,
            FSM::transition_t(State::kDataSent, State::kResetSent));
  fsm_.When(TriggerType::kRecvAllACKs,
            FSM::transition_t(State::kDataSent, State::kDataRecvd));
  fsm_.When(TriggerType::kRecvAck,
            FSM::transition_t(State::kResetSent, State::kResetRecvd));
  fsm_.On(State::kReady, std::bind(&SendSide::OnReady, shared_from_this()));
  fsm_.On(State::kSend, std::bind(&SendSide::OnSend, shared_from_this()));
  fsm_.On(State::kDataSent,
          std::bind(&SendSide::OnDataSent, shared_from_this()));
  fsm_.On(State::kResetSent,
          std::bind(&SendSide::OnResetSent, shared_from_this()));
  fsm_.On(State::kDataRecvd,
          std::bind(&SendSide::OnDataRecvd, shared_from_this()));
  fsm_.On(State::kResetRecvd,
          std::bind(&SendSide::OnResetRecvd, shared_from_this()));
  routine_ = std::shared_ptr<AsynRoutine>(reinterpret_cast<AsynRoutine*>(
      new Coroutine(CoSendSide, &*shared_from_this())));
  cmdQueue_ = std::shared_ptr<CommandQueue<CmdSendSide>>(
      reinterpret_cast<CommandQueue<CmdSendSide>*>(
          new CoCmdQueue<CmdSendSide>(routine_)));
  routine_->Resume();  // Create Stream (Sending)
}

void SendSide::ConsumeCmd() {
  auto cmd = cmdQueue_->WaitAndExecuteCmds(shared_from_this());
}

void SendSide::WriteData(std::shared_ptr<std::stringstream> data) {
  send_buffer_ << data->str();
}

void SendSide::EndStream() {}

void SendSide::ResetStream() {}

}  // namespace moss
