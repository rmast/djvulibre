me: C/C++ CI

on:
  push:
    branches: [ "main" ]
  pull_request:
    branches: [ "main" ]
    
jobs:
  build:

    runs-on: ubuntu-latest

    steps:
    - name: 'install deps'
      run: "sudo apt install -y libfreetype6-dev gsfonts libmagickwand-dev imagemagick"
    - name: 'install deps2'
      run: which convert
