# Conda-Recipe
This recipe is copied over from conda-forge/jpype1-feedstock and should be kept in close sync with it.
By doing so, we ensure that upcoming releases onc Conda-Forge can be achieved more easily.

## Prerequisites
We recommend using `boa` and the provided wrapper `mambabuild` to speed up resolving the dependencies.

conda install -c conda-forge boa conda-forge-pinning conda-verify


## Run it
This should use current compiler configurations (and due to this runtime restrictions for Python and libcxx etc.)
from conda-forge itself.

    conda mambabuild conda_recipe --variant-config-files $CONDA_PREFIX/conda_build_config.yaml
