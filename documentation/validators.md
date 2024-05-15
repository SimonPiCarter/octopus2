# Validators

Description of the validator

## Sequence

Validators run during the PreStore phase.
They must all have the Validator tag to ease replay out of classic pipelines.

## List

### HitPoint

- Ensure that HitPoint >= 0
- Ensure that HitPoint <= HitPointMax if applicable

### HitPointMax

- Ensure that HitPointMax >= 1
