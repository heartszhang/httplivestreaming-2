#pragma once

inline bool ok(HRESULT hr) { return SUCCEEDED(hr); }
inline bool failed(HRESULT hr) { return FAILED(hr); }
