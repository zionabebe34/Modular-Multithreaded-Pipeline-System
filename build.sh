#!/usr/bin/env bash
set -e

echo "Building analyzer and plugins"
rm -rf output

# Ensure output and plugins directories exist
mkdir -p output
mkdir -p plugins

# Build analyzer
echo "Building main analyzer..."
gcc -std=c11 -Wall -Wextra -pthread -ldl -Iplugins -Iplugins/sync \
    -o output/analyzer main.c

# List of plugins
PLUGINS="logger uppercaser rotator flipper expander typewriter"

# Build each plugin
for plugin in $PLUGINS; do
    echo "Building plugin: $plugin"
    gcc -std=c11 -Wall -Wextra -fPIC -shared -Iplugins -Iplugins/sync \
        -o output/${plugin}.so \
        plugins/${plugin}.c \
        plugins/plugin_common.c \
        plugins/sync/monitor.c \
        plugins/sync/consumer_producer.c \
        -lpthread -ldl
done

echo "build.sh completed successfully."