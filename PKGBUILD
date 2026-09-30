# Maintainer: Zoheb Shujauddin <zoheb2424@gmail.com>
pkgname=toymaker
pkgver=0.5.5
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
options=('!lto')
install=
changelog=
source=(
    "$pkgname-$pkgver-Source.tar.gz"
)
noextract=()
sha256sums=('13fb58e6f7011bf36789003847cdb37b747da66f2dda238f613207e1ef8b3cd2')
validpgpkeys=()

build() {
    local cmake_options=(
        -B build
        -S $pkgname-$pkgver-Source
        -W no-author
        -D CMAKE_INSTALL_PREFIX=/usr
    )
    cmake "${cmake_options[@]}"
    cmake --build build
}

package() {
    DESTDIR="$pkgdir/" cmake --install build
}
