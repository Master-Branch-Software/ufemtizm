#!/bin/bash
export QT_PLUGIN_PATH="/usr/lib/Ufemtizm/plugins"
export LD_LIBRARY_PATH="/usr/lib/Ufemtizm:$LD_LIBRARY_PATH"
exec "/usr/lib/Ufemtizm/Ufemtizm" "$@"
