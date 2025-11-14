#!/bin/bash
export QT_PLUGIN_PATH="/usr/lib/UnfuckMyTimeZoneMath/plugins"
export LD_LIBRARY_PATH="/usr/lib/UnfuckMyTimeZoneMath:$LD_LIBRARY_PATH"
exec "/usr/lib/UnfuckMyTimeZoneMath/UnfuckMyTimeZoneMath" "$@"
