cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Αφαιρέσαμε το MINIMAL_BUILD για να βρει το NimBLE
project(esp_plants)