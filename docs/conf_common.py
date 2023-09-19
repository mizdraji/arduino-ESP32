# ---------------------------------------------------------------

from esp_docs.conf_docs import *  # noqa: F403,F401

languages = ["en"]

idf_targets = [
    "esp32",
    "esp32s2",
    "esp32s3",
    "esp32c3",
    "esp32c6",
    "esp32h2",
]

# link roles config
github_repo = "espressif/arduino-esp32"

# context used by sphinx_idf_theme
html_context["github_user"] = "espressif"
html_context["github_repo"] = "arduino-esp32"

html_static_path = ["../_static"]

# Conditional content

extensions += ['sphinx_copybutton',
               'sphinx_tabs.tabs',
               'esp_docs.esp_extensions.dummy_build_system',
               ]

ESP32_DOCS = [
    "index.rst",
]

# Extra options required by sphinx_idf_theme
project_slug = "arduino-esp32"

versions_url = "./_static/arduino-esp32_versions.js"
