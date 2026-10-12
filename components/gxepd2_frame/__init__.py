import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID

DEPENDENCIES = ["esp8266"]
ns = cg.esphome_ns.namespace("gxepd2_frame")
GxEPD2Frame = ns.class_("GxEPD2Frame", cg.Component)
CONFIG_SCHEMA = cv.All(
    cv.Schema({cv.GenerateID(): cv.declare_id(GxEPD2Frame)}).extend(cv.COMPONENT_SCHEMA),
    cv.only_on_esp8266,
    cv.only_with_arduino,
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    # ESPHome defaults to LDF off; GFX itself includes BusIO headers.
    # Resolve library-to-library includes even when YAML omits this option.
    cg.add_platformio_option("lib_ldf_mode", "chain+")
    cg.add_library("SPI", None)
    cg.add_library("Wire", None)  # Adafruit GFX/BusIO headers require Wire.h.
    cg.add_library("zinggjm/GxEPD2", "1.6.9")
    cg.add_library("adafruit/Adafruit GFX Library", "1.12.4")
    cg.add_library("adafruit/Adafruit BusIO", "1.17.4")
