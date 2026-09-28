# Maintainer: Zoheb Shujauddin <zoheb2424@gmail.com>
pkgname=toymaker
pkgver=0.5.4
pkgrel=1
epoch=
pkgdesc="A small open source 3D game engine built on C++, OpenGL, and SDL."
arch=('x86_64')
url="https://www.github.com/raynmetal/toymaker"
license=('MIT')
groups=()
depends=(
    'sdl3'
    'sdl3_mixer'
    'sdl3_ttf'
    'sdl3_image'
    'glew'
    'glm'
    'nlohmann-json'
    'assimp'
    'doctest'
)
makedepends=('cmake')
checkdepends=()
optdepends=()
provides=()
conflicts=()
replaces=()
backup=()
options=()
install=
changelog=
source=(
    "$pkgname-$pkgver-Source.tar.gz"
)
noextract=()
sha256sums=('d02241f0f758eef23c4c9afaebaaabc2cc28eb39e6c18572334fd1451d8491aa')
validpgpkeys=()

build() {
    local cmake_options=(
        -B build
        -S $pkgname-$pkgver-Source
        -W no-author
        -D CMAKE_BUILD_TYPE=None
        -D CMAKE_INSTALL_PREFIX=/usr
    )
    cmake "${cmake_options[@]}"
    cmake --build build
}

package() {
    DESTDIR="$pkgdir/" cmake --install build
}
