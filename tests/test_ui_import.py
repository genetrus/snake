import importlib


def test_ui_imports():
    importlib.import_module("money_map.ui.app")
    importlib.import_module("money_map.ui.views.recommendations")
    importlib.import_module("money_map.ui.views.plan")
