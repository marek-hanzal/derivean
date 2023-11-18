import {type Infer}          from "@use-pico/extras";
import {type ProducerSchema} from "../schema/ProducerSchema";

export class DependencyError extends Error {
    public dependencies: Infer.Entity<ProducerSchema>[];

    constructor(message: string, dependencies: Infer.Entity<ProducerSchema>[]) {
        super(message);
        this.dependencies = dependencies;
    }
}
