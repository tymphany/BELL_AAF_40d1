#!/usr/bin/env python
# Automatically generated input script for dfu_file_generator.py

dfu_image_parameters = {
    "gen_flash_image": "True",
    "bank": "bank1"
}

flash_device_parameters = {
    "block_size": 65536,
    "boot_block_size": 65536,
    "alt_image_offset": 4194304
}

host_tools_parameters = {
    "devkit": r"C:\qtil\ADK_Toolkit_0.9.1.9_x64",
    "NvsCmd": r"C:\qtil\ADK_Toolkit_0.9.1.9_x64\tools\bin\nvscmd.exe",
    "SecurityCmd": r"C:\qtil\ADK_Toolkit_0.9.1.9_x64\tools\bin\SecurityCmd.exe",
    "UpgradeFileGen": r"C:\qtil\ADK_Toolkit_0.9.1.9_x64\tools\bin\UpgradeFileGen.exe",
    "crescendo_upd_config": r"E:\QCC5121\EVK\qcc512x-qcc302x-40d1\earbud\workspace\QCC3026-AA_DEV-BRD_R2-AA\dfu\20200430201813\scripts\20200430201813.upd",
    "dfu_dir": r"E:\QCC5121\EVK\qcc512x-qcc302x-40d1\earbud\workspace\QCC3026-AA_DEV-BRD_R2-AA\dfu\20200430201813\output",
    "folder_for_rsa_files": r"E:/QCC5121/EVK/qcc512x-qcc302x-40d1/earbud/workspace/QCC3026-AA_DEV-BRD_R2-AA/dfu",
}

flash0 = {
    "flash_device": flash_device_parameters,
    "dfu_image": dfu_image_parameters,
    "host_tools": host_tools_parameters,
    "chip_type": "QCC512X",
    "encrypt": False,
    "signing_mode": "header",
    "layout": [
        ("curator_fs", {
           "src_file" : r"E:\QCC5121\EVK\qcc512x-qcc302x-40d1\earbud\workspace\QCC3026-AA_DEV-BRD_R2-AA\dfu\20200430201813\input\curator_config_filesystem.xuv",
            "src_file_signed": False,
            "authenticate": False,
            "capacity": 65536,
            }),
        ("apps_p0", {
           "src_file" : r"E:\QCC5121\EVK\qcc512x-qcc302x-40d1\earbud\workspace\QCC3026-AA_DEV-BRD_R2-AA\dfu\20200430201813\input\apps_p0_firmware.xuv",
            "src_file_signed": True,
            "authenticate": True,
            "capacity": 786432,
            }),
        ("apps_p1", {
           "src_file" : r"E:\QCC5121\EVK\qcc512x-qcc302x-40d1\earbud\workspace\QCC3026-AA_DEV-BRD_R2-AA\dfu\20200430201813\input\earbud.xuv",
            "authenticate": False,
            "capacity": 786432,
            }),
        ("device_ro_fs", {
            "authenticate": False,
            "capacity": 65536,
            "inline_auth_hash": True,
            }),
        ("rw_config", {
            "capacity": 131072,
            }),
        ("rw_fs", {
            "capacity": 262144,
            }),
        ("ro_cfg_fs", {
           "src_file" : r"E:\QCC5121\EVK\qcc512x-qcc302x-40d1\earbud\workspace\QCC3026-AA_DEV-BRD_R2-AA\dfu\20200430201813\input\firmware_config_filesystem_dfu.xuv",
            "authenticate": False,
            "capacity": 262144,
            }),
        ("ro_fs", {
           "src_file" : r"E:\QCC5121\EVK\qcc512x-qcc302x-40d1\earbud\workspace\QCC3026-AA_DEV-BRD_R2-AA\dfu\20200430201813\input\customer_ro_filesystem_dfu.xuv",
            "authenticate": False,
            "capacity": 1703936,
            }),
    ]
}

flash1 = {
    "flash_device": flash_device_parameters,
    "layout": []
}
