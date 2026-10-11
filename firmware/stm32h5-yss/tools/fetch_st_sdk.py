#!/usr/bin/env python3
"""
STM32CubeH5 구성 저장소 / CMSIS 에서 이 프로젝트가 쓰는 파일만 골라 lib 로 가져온다.

STM32CubeH5 를 통째로 받거나 CubeMX 를 설치하지 않는다. 필요한 파일만 **커밋 SHA 를
고정해서** GitHub raw 로 내려받는다. 받은 결과는 저장소에 커밋해 두므로 평소 빌드에는
이 스크립트도, 네트워크도 필요 없다. 버전을 올리거나 파일을 추가할 때만 돌린다.

    python3 tools/fetch_st_sdk.py            # 목록대로 다시 받는다 (덮어쓴다)
    python3 tools/fetch_st_sdk.py --check    # 받은 파일이 목록과 같은지만 확인

표준 라이브러리만 쓴다 (Windows / Linux / macOS 공통).
결과 목록은 lib/st/SOURCES.md 에 남는다.
"""
import argparse
import hashlib
import sys
import urllib.request
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent          # firmware/stm32h5-yss
LIB = ROOT / 'lib'

#-- 커밋을 고정한다. 태그 v1.7.0 이 가리키는 커밋이다(태그는 옮겨질 수 있다).
#
REPOS = {
    'device': ('STMicroelectronics/cmsis-device-h5',      '884b8dc78e41cbfca008363342b17f4a9e8641f7'),  # v1.7.0
    'hal':    ('STMicroelectronics/stm32h5xx-hal-driver', '4b27033297df34437bfef6b3451d9b102d4ed9e5'),  # v1.7.0
    'cmsis':  ('ARM-software/CMSIS_6',                    'v6.4.0'),
}

#-- LL 드라이버. 헤더는 대부분 static inline 이라 .c 는 필요한 것만 받는다.
#   yss 드라이버(lib/yss/src/targets/st)와 bsp 가 쓰는 것만 둔다.
#
LL_HEADERS = ['bus', 'cortex', 'gpio', 'icache', 'pwr', 'rcc', 'system', 'tim', 'usart', 'utils']
LL_SOURCES = ['rcc', 'utils']

#-- (저장소, 원본 경로, 저장 경로 — lib 기준)
#
FILES = [
    #-- CMSIS Core (M-profile, CM33 만)
    *[('cmsis', f'CMSIS/Core/Include/{f}', f'cmsis/Core/Include/{f}') for f in [
        'cmsis_compiler.h', 'cmsis_gcc.h', 'cmsis_version.h', 'tz_context.h',
        'core_cm33.h',
        'm-profile/cmsis_gcc_m.h', 'm-profile/armv8m_mpu.h', 'm-profile/armv8m_pmu.h',
        'm-profile/armv81m_pac.h',
    ]],
    ('cmsis', 'LICENSE', 'cmsis/LICENSE'),

    #-- 디바이스: 레지스터 헤더, startup
    #   system_stm32h5xx.c 는 받지 않는다. yss msp 가 SystemInit 을 제공한다(docs/12).
    *[('device', f'Include/{f}', f'st/device/Include/{f}') for f in [
        'stm32h5xx.h', 'stm32h563xx.h', 'system_stm32h5xx.h', 'partition_stm32h5xx.h',
    ]],
    ('device', 'Source/Templates/gcc/startup_stm32h563xx.s', 'st/device/gcc/startup_stm32h563xx.s'),
    # 링커 스크립트는 참고용이다. 빌드는 src/bsp/ldscript 의 우리 스크립트를 쓴다.
    ('device', 'Source/Templates/gcc/linker/STM32H563xx_FLASH.ld', 'st/device/gcc/STM32H563xx_FLASH.ld.ref'),
    ('device', 'LICENSE.md', 'st/device/LICENSE.md'),

    #-- LL
    *[('hal', f'Inc/stm32h5xx_ll_{f}.h', f'st/ll/Inc/stm32h5xx_ll_{f}.h') for f in LL_HEADERS],
    *[('hal', f'Src/stm32h5xx_ll_{f}.c', f'st/ll/Src/stm32h5xx_ll_{f}.c') for f in LL_SOURCES],
    ('hal', 'LICENSE.md', 'st/ll/LICENSE.md'),
]


def raw_url(repo_key, path):
    repo, ref = REPOS[repo_key]
    return f'https://raw.githubusercontent.com/{repo}/{ref}/{path}'


def fetch(url):
    with urllib.request.urlopen(url) as r:
        return r.read()


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--check', action='store_true', help='내려받기만 하고 비교한다')
    args = ap.parse_args()

    lines = []
    diff = 0
    for repo_key, src, dst in FILES:
        data = fetch(raw_url(repo_key, src))
        out = LIB / dst
        sha = hashlib.sha256(data).hexdigest()[:12]
        if args.check:
            if not out.exists() or out.read_bytes() != data:
                print(f'다름  {dst}')
                diff += 1
        else:
            out.parent.mkdir(parents=True, exist_ok=True)
            out.write_bytes(data)
        lines.append(f'| `{dst}` | {REPOS[repo_key][0]} | `{src}` | `{sha}` |')
        print(f'{"확인" if args.check else "받음"}  {dst}')

    if args.check:
        print(f'\n{len(FILES)}개 중 {diff}개 다름')
        return 1 if diff else 0

    head = [
        '# lib 출처',
        '',
        '`tools/fetch_st_sdk.py` 가 만든 목록이다. 손으로 고치지 않는다.',
        '',
        '| 저장소 | ref |',
        '|---|---|',
        *[f'| {r} | `{ref}` |' for r, ref in REPOS.values()],
        '',
        '| 저장 경로 (lib 기준) | 저장소 | 원본 경로 | sha256 앞 12자 |',
        '|---|---|---|---|',
    ]
    (LIB / 'st' / 'SOURCES.md').write_text('\n'.join(head + sorted(lines)) + '\n')
    print(f'\n{len(FILES)}개 파일, 목록: lib/st/SOURCES.md')
    return 0


if __name__ == '__main__':
    sys.exit(main())
