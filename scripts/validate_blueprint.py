"""Validate against real Home Assistant schemas, without connecting to a device."""
from pathlib import Path
from tempfile import TemporaryDirectory
from homeassistant.core import HomeAssistant
from homeassistant.components.blueprint.models import Blueprint, BlueprintInputs
from homeassistant.components.blueprint.schemas import BLUEPRINT_SCHEMA
from homeassistant.components.automation.config import PLATFORM_SCHEMA
from homeassistant.components.script.config import SCRIPT_ENTITY_SCHEMA
from homeassistant.components.shell_command import CONFIG_SCHEMA as SHELL_SCHEMA
from homeassistant.util.yaml import load_yaml

import asyncio

async def main():
    temp = TemporaryDirectory(prefix="epaper-schema-")
    hass = HomeAssistant(temp.name)
    path = Path(__file__).resolve().parents[1] / "homeassistant/epaper_test_blueprint.yaml"
    blueprint = Blueprint(load_yaml(str(path)), expected_domain="automation", schema=BLUEPRINT_SCHEMA)
    inputs = BlueprintInputs(blueprint, {
        "alias": "Schema validation only",
        "id": "epaper_schema_validation",
        "use_blueprint": {
            "path": "epaper_test_blueprint.yaml",
            "input": {
                "test_button": "button.epaper_test",
                "rendering_sensor": "binary_sensor.epaper_rendering",
                "heap_sensor": "sensor.epaper_heap",
            },
        },
    })
    inputs.validate()
    config = PLATFORM_SCHEMA(inputs.async_substitute())
    print("Blueprint metadata, inputs and automation/action schema validated.")
    package = load_yaml(str(path.parent / "epaper_images_package.yaml"))
    SHELL_SCHEMA(package)
    SCRIPT_ENTITY_SCHEMA(package["script"]["epaper_eigenes_bild"])
    print("Image preparation package shell-command and script schemas validated.")
    eink = load_yaml(str(path.parent / "eink_dashboard_package.yaml"))
    SHELL_SCHEMA(eink)
    SCRIPT_ENTITY_SCHEMA(eink["script"]["epaper_eink_dashboard_aktualisieren"])
    PLATFORM_SCHEMA(eink["automation"][0])
    from homeassistant.components.input_text import CONFIG_SCHEMA as INPUT_TEXT_SCHEMA
    INPUT_TEXT_SCHEMA(eink)
    from homeassistant.helpers.template import Template
    command = eink["shell_command"]["epaper_dashboard_vorbereiten"]
    import base64
    dangerous = 'http://host/image.png?x=";$(echo unexpected);`echo unexpected`'
    hass.states.async_set("input_text.epaper_dashboard_png_url", dangerous)
    rendered = Template(command, hass).async_render()
    encoded = base64.b64encode(dangerous.encode()).decode()
    assert encoded in rendered and "$(" not in rendered and "`" not in rendered
    print("E-Ink Dashboard package and shell-safe URL encoding validated.")
    print("No device actions executed; live behavior remains a hardware/HA test.")
    await hass.async_stop()
    temp.cleanup()

asyncio.run(main())
