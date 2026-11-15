# Dear ImGui

Core and the compiled Win32 / DirectX 12 backends are vendored unmodified from
[v1.92.6-docking](https://github.com/ocornut/imgui/tree/v1.92.6-docking).
The upstream MIT license is included in `LICENSE.txt`.

The vendored files include the core and the Win32 and DirectX 12 backends.

The engine enables docking, platform viewports, and DPI font scaling. DX12
initialization supplies the engine command queue and a dedicated 64-slot SRV
allocator for dynamic font textures. Secondary viewport work is submitted after
the main draw list, before presentation and the engine frame fence. The official
backends own the secondary windows, swap chains, and their synchronization.
The ImGui frame-resource count is at least two because the backend also uses it
as the flip-model swap-chain buffer count; the engine itself currently uses one
frame in flight.

Manual validation: open Inspector, drag its title bar outside the main window,
resize/edit it, dock it back, toggle F1, close/reopen it, and use
`... > Bring inspector back`.
Also check mixed-DPI monitors, minimize/restore, saved layouts on restart, and
shutdown with a detached window still open on the target machine.
