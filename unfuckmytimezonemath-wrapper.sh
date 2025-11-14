#!/bin/bash
export QT_PLUGIN_PATH="/usr/local/lib/UnfuckMyTimeZoneMath/plugins"
export LD_LIBRARY_PATH="/usr/local/lib/UnfuckMyTimeZoneMath:$LD_LIBRARY_PATH"
exec "/usr/local/lib/UnfuckMyTimeZoneMath/UnfuckMyTimeZoneMath" "$@"
