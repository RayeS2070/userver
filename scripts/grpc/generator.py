#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# pylint: disable=invalid-name

"""
This script is a Protobuf protoc plugin that generates userver asynchronous
wrappers for gRPC clients.

For each `path/X.proto` file that contains gRPC services, we generate
`path/X_{client,handler}.usrv.pb.{hpp,cpp}` using
`{client,handler}.usrv.pb.{hpp,cpp}.jinja` templates.
"""

import enum
import itertools
import os
import sys
from typing import Iterable
from typing import Optional
from typing import Tuple

from google.protobuf.compiler import plugin_pb2 as plugin
import jinja2


_AUTOGEN_EMPTY_HEADER = '\n'.join([
    '// THIS FILE IS AUTOGENERATED, DO NOT EDIT!',
    '',
    '// This file is empty intentionally',
    '',
])


class Mode(enum.Enum):
    Service = enum.auto()
    Client = enum.auto()
    Both = enum.auto()

    def is_service(self) -> bool:
        return self == self.Service

    def is_client(self) -> bool:
        return self == self.Client

    def is_both(self) -> bool:
        return self == self.Both


def _grpc_to_cpp_name(in_str):
    return in_str.replace('.', '::')


def _to_package_prefix(package):
    return f'{package}.' if package else ''


class _CodeGenerator:
    def __init__(
        self,
        proto_file,
        response: plugin.CodeGeneratorResponse,
        jinja_env: jinja2.Environment,
        mode: Mode,
        skip_files_wo_service: bool,
    ) -> None:
        self.proto_file = proto_file
        self.response = response
        self.jinja_env = jinja_env
        self.mode = mode
        self.skip_files_wo_service = skip_files_wo_service

    def run(self) -> None:
        if self.proto_file.service:
            self._generate_code_with_service()
        elif not self.skip_files_wo_service:
            self._generate_code_empty()

    def _generate_code_with_service(self) -> None:
        data = {
            'source_file': self.proto_file.name,
            'source_file_without_ext': self._proto_file_stem(),
            'package_prefix': _to_package_prefix(self.proto_file.package),
            'namespace': _grpc_to_cpp_name(self.proto_file.package),
            'services': list(self.proto_file.service),
        }

        for file_type, file_ext in self._iter_src_files():
            template_name = f'{file_type}.usrv.{file_ext}.jinja'
            template = self.jinja_env.get_template(template_name)

            file = self.response.file.add()
            file.name = self._proto_file_dest(
                file_type=file_type, file_ext=file_ext,
            )
            file.content = template.render(proto=data)

    def _generate_code_empty(self) -> None:
        for file_type, file_ext in self._iter_src_files():
            file = self.response.file.add()
            file.name = self._proto_file_dest(
                file_type=file_type, file_ext=file_ext,
            )
            file.content = _AUTOGEN_EMPTY_HEADER

    def _iter_src_files(self) -> Iterable[Tuple[str, str]]:
        if self.mode.is_service():
            src_files = ['service']
        elif self.mode.is_client():
            src_files = ['client']
        elif self.mode.is_both():
            src_files = ['client', 'service']
        else:
            raise Exception(f'Unknown mode {self.mode}')

        return itertools.product(src_files, ['hpp', 'cpp'])

    def _proto_file_stem(self) -> str:
        return self.proto_file.name.replace('.proto', '')

    def _proto_file_dest(self, file_type: str, file_ext: str) -> str:
        return f'{self._proto_file_stem()}_{file_type}.usrv.pb.{file_ext}'


def generate(
    loader: jinja2.BaseLoader, mode: Mode, skip_files_wo_service: bool,
) -> None:
    data = sys.stdin.buffer.read()

    request = plugin.CodeGeneratorRequest()
    request.ParseFromString(data)

    response = plugin.CodeGeneratorResponse()
    if hasattr(response, 'FEATURE_PROTO3_OPTIONAL'):
        setattr(
            response,
            'supported_features',
            getattr(response, 'FEATURE_PROTO3_OPTIONAL'),
        )

    jinja_env = jinja2.Environment(
        loader=loader,
        trim_blocks=True,
        lstrip_blocks=True,
        # We do not render HTML pages with Jinja. However the gRPC data could
        # be shown on web. Assuming that the generation should not deal with
        # HTML special characters, it is safer to turn on autoescaping.
        autoescape=True,
    )
    jinja_env.filters['grpc_to_cpp_name'] = _grpc_to_cpp_name

    # pylint: disable=no-member
    for proto_file in request.proto_file:
        _CodeGenerator(
            jinja_env=jinja_env,
            proto_file=proto_file,
            response=response,
            mode=mode,
            skip_files_wo_service=skip_files_wo_service,
        ).run()

    output = response.SerializeToString()
    sys.stdout.buffer.write(output)


def main(
    loader: Optional[jinja2.BaseLoader] = None,
    mode: Mode = Mode.Both,
    skip_files_wo_service: bool = True,
) -> None:
    if loader is None:
        loader = jinja2.FileSystemLoader(
            os.path.join(
                os.path.dirname(os.path.abspath(__file__)), 'templates',
            ),
        )

    generate(
        loader=loader, mode=mode, skip_files_wo_service=skip_files_wo_service,
    )


if __name__ == '__main__':
    main()
