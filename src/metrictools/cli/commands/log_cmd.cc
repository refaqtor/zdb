/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2016 Paul Asmuth, Laura Schlimmer, FnordCorp B.V.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <iostream>
#include <metrictools/cli/commands/log_cmd.h>
#include <metrictools/util/flagparser.h>

namespace fnordmetric {

ReturnCode LogCommand::execute(
    CLIContext* ctx,
    const std::vector<std::string>& argv) {
  FlagParser flags;

  auto flags_rc = flags.parseArgv(argv);
  if (!flags_rc.isSuccess()) {
    return flags_rc;
  }

  return ReturnCode::success();
}

const std::string& LogCommand::getName() const {
  static const std::string kName = "log";
  return kName;
}

const std::string& LogCommand::getDescription() const {
  static const std::string kDescription =
      "Log the historical values for one or more metric instances";

  return kDescription;
}

void LogCommand::printHelp() const {
  std::cerr <<
      "Usage: $ metricctl log [<options>] <metric>\n"
      "  -F, --filter <expr>       The tree path of the metric.\n"
      "\n"
      "Examples:\n"
      "  $ metrictl log load_average\n"
      "  $ metrictl log load_average -F 'hostname=localhost'\n";
}

} // namespace fnordmetric

