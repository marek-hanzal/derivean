import {type GraphSchema}          from "@use-pico/diagram";
import {withDullSchema}            from "@use-pico/dull-stuff";
import {type ProducerSchema}       from "../schema/ProducerSchema";
import {type ProductionTimeSchema} from "../schema/ProductionTimeSchema";
import {type IProducerProcess}     from "./IProducerProcess";
import {type IProducerSnapshot}    from "./IProducerSnapshot";

export interface IProducerService {
    /**
     * Number of cycles passed by the given process
     */
    cycles(process: IProducerProcess): number;

    /**
     * Compute a producer process into a production snapshot.
     *
     * Snapshots are used to compute the final result of the production and
     * are used to update the inventory.
     */
    process(process: IProducerProcess): IProducerSnapshot;

    /**
     * Get a minimal amount of time a producer needs to finish a cycle.
     */
    timeOf(producerId: string): Promise<ProductionTimeSchema.Type>;

    /**
     * Get a list of all dependencies of the given producer.
     *
     * Stack is used to prevent circular dependencies.
     */
    dependencies(producerId: string, stack?: string[]): Promise<withDullSchema.Infer.Entity<ProducerSchema>[]>;

    graph(producerId: string): Promise<GraphSchema.Type>;
}
