#include "mprpccontroller.h"

MprpcController::MprpcController() {
  failed_ = false;
  errText_ = "";
}

void MprpcController::Reset() {
  failed_ = false;
  errText_ = "";
}

bool MprpcController::Failed() const { return failed_; }

std::string MprpcController::ErrorText() const { return errText_; }

void MprpcController::SetFailed(const std::string& reason) {
  failed_ = true;
  errText_ = reason;
}

// 目前未实现具体的功能
void MprpcController::StartCancel() {}
bool MprpcController::IsCanceled() const { return false; }
void MprpcController::NotifyOnCancel(google::protobuf::Closure* callback) {}