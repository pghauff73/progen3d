# Patch note

`MCSMv2_0_1_to_2_2.patch` is an architectural source diff. MCSMv2.2 deliberately retains the MCSMv2.0.1 implementation as `mcsmv201_base.py` and imports it as the integrity/base layer, so the release must keep both files when using the patched architecture.
