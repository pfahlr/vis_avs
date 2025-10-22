# Ways to improve `ci.yml`

Nice—those three cover the biggest wins. What’s *left* (ordered by impact):
<strike>
[x] - 1. Split build & tests into separate jobs (parallelize wall time)

* Build once → upload artifact (binaries + CMake/Ninja metadata, exclude `*.o`/`*.a`).
* Fan out multiple `tests` jobs that download the artifact and run shards in parallel.
* Build step: `cmake --build build -- -k 0`
* Test step: `ctest --test-dir build --output-on-failure --parallel ${{ env.CMAKE_BUILD_PARALLEL_LEVEL }}`
</strike>

[ ] - 2. Gate Fedora to main/nightly

* Keep PRs on Ubuntu for speed.
* Add `if: github.ref == 'refs/heads/main' || github.event_name == 'schedule'` to Fedora job, or move Fedora to a nightly workflow (`on: schedule`).

[ ] - 3. Pre-baked CI images (big cold-start win)

* Build & push `ghcr.io/<owner>/<repo>-ci:ubuntu-22.04` and `:fedora-39` with cmake/ninja/ccache preinstalled.
* Switch jobs to `container: ghcr.io/...` and remove apt/dnf steps.

[ ] - 4. Make ctest explicitly parallel (if you haven’t already)

* You set `CMAKE_BUILD_PARALLEL_LEVEL`, but ensure tests use it:

  ```
  ctest --test-dir build --output-on-failure --parallel ${{ env.CMAKE_BUILD_PARALLEL_LEVEL }}
  ```

5. Add ccache stats + baselining

* Before configure: `ccache --zero-stats || true`
* After tests: `ccache -s || true`
* Acceptance: non-zero hits on second run.

6. Improve cache key hygiene for the compiler cache

* Include compiler version in the **ccache** key to avoid mixing GCC/Clang outputs across runners:

  * Detect:

    ```bash
    echo "gcc=$(/usr/bin/gcc -dumpfullversion -dumpversion || echo none)" >> "$GITHUB_OUTPUT"
    echo "clang=$(clang --version | head -n1 | awk '{print $3}')" >> "$GITHUB_OUTPUT" || true
    ```
  * Key:

    ```
    ccache-${{ runner.os }}-${{ matrix.distro }}-gcc${{ steps.cc.outputs.gcc }}-clang${{ steps.cc.outputs.clang }}
    ```
* (Optional) Add `CCACHE_BASEDIR: ${{ github.workspace }}` and `CCACHE_NOHASHDIR: 1` to improve hit rates across path changes.

7. Optional: switch to `sccache` with a remote backend

* For shared cache hits across runners/branches, wire S3/MinIO/Redis and drop `actions/cache` for the compiler cache.

8. Micro-optimizations

* Skip `apt-get update`/`dnf makecache` when pre-baked images are in place.
* Keep FetchContent cache, and consider `-DFETCHCONTENT_FULLY_DISCONNECTED=ON` after restore (if your CMake logic allows) to avoid network checks.

**Quick acceptance checks**

* PRs: Ubuntu job time drops further; Fedora runs only on main/nightly.
* Second run: compiler cache shows hits; ctest runs in parallel; overall wall time reduced vs your current setup.
* No cache pollution (keys include OS/distro/compiler).
