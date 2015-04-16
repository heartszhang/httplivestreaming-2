#pragma once
inline void ThrowIfError(HRESULT hr)
{
    if (FAILED(hr))
    {
        throw ref new Platform::COMException(hr);
    }
}

inline void ThrowException(HRESULT hr)
{
    assert(FAILED(hr));
    throw ref new Platform::COMException(hr);
}