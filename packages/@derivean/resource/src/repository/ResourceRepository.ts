import {AbstractRepository}   from "@use-pico/repository";
import {ResourceSourceSchema} from "../schema/ResourceSourceSchema";

export class ResourceRepository extends AbstractRepository<ResourceSourceSchema> {
    constructor() {
        super(ResourceSourceSchema);
    }
}
