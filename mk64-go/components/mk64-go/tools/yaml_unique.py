"""Read decomp YAML while retaining entries whose mapping keys were repeated."""

import yaml


class PreserveDuplicateKeysLoader(yaml.SafeLoader):
    pass


def construct_mapping(loader, node):
    loader.flatten_mapping(node)
    mapping = {}
    for key_node, value_node in node.value:
        key = loader.construct_object(key_node)
        value = loader.construct_object(value_node)
        unique = key
        duplicate = 2
        while unique in mapping:
            unique = f"{key}__duplicate_{duplicate}"
            duplicate += 1
        mapping[unique] = value
    return mapping


PreserveDuplicateKeysLoader.add_constructor(
    yaml.resolver.BaseResolver.DEFAULT_MAPPING_TAG, construct_mapping
)


def load(text):
    return yaml.load(text, Loader=PreserveDuplicateKeysLoader)
