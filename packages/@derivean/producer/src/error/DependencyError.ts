import {withDullSchema} from "@use-pico/dull-stuff";
import {ProducerSchema} from "../schema/ProducerSchema";

export class DependencyError extends Error {
    public dependencies: withDullSchema.Infer.Entity<ProducerSchema>[];

    constructor(message: string, dependencies: withDullSchema.Infer.Entity<ProducerSchema>[]) {
        super(message);
        this.dependencies = dependencies;
    }
}
