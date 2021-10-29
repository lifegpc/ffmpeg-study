from typing import List, Union, Dict, Tuple
def version() -> List[int]: ...
class AVDict:
    def __delitem__(self, key): ...
    def __getitem__(self, key) -> Union[str, bytes, None]: ...
    def __len__(self) -> int: ...
    def __setitem__(self, key, value): ...
    def __str__(self) -> str: ...
    def copy(self, flags: int = 0) -> AVDict: ...
    def delete(self, key: bytes, flags: int = 0): ...
    def get(self, key: bytes, flags: int = 0) -> Union[str, bytes, None]: ...
    def set(self, key: bytes, value: bytes, flags: int = 0): ...
    def to_dict(self) -> Dict[Union[bytes, str], Union[bytes, str]]: ...
    def to_list(self) -> List[Tuple[Union[bytes, str], Union[bytes, str]]]: ...
class Chapter:
    @property
    def id(self) -> int: ...
    @property
    def start(self) -> float: ...
    @property
    def end(self) -> float: ...
    @property
    def metadata(self) -> Union[AVDict, None]: ...
    @property
    def title(self) -> Union[bytes, str, None]: ...
class StreamInfo:
    @property
    def bit_rate(self) -> Union[int, None]: ...
    @property
    def channels(self) -> Union[int, None]: ...
    @property
    def duration(self) -> Union[float, None]: ...
    @property
    def height(self) -> Union[int, None]: ...
    @property
    def id(self) -> int: ...
    @property
    def index(self) -> int: ...
    @property
    def is_audio(self) -> bool: ...
    @property
    def is_video(self) -> bool: ...
    @property
    def metadata(self) -> Union[AVDict, None]: ...
    @property
    def sample_aspect_ratio(self) -> Union[float, None]: ...
    @property
    def sample_rate(self) -> Union[int, None]: ...
    @property
    def width(self) -> Union[int, None]: ...
class VideoInfo:
    @property
    def chapters(self) -> List[Chapter]: ...
    @property
    def duration(self) -> Union[float, None]: ...
    @property
    def meta(self) -> Union[AVDict, None]: ...
    @property
    def mime_type(self) -> Union[bytes, str, None]: ...
    @property
    def nb_chapters(self) -> int: ...
    @property
    def nb_streams(self) -> int: ...
    @property
    def ok(self) -> bool: ...
    def parse(self, s: Union[str, bytes]) -> bool: ...
    @property
    def streams(self) -> List[StreamInfo]: ...
    @property
    def type_long_name(self) -> Union[bytes, str, None]: ...
    @property
    def type_name(self) -> Union[bytes, str, None]: ...