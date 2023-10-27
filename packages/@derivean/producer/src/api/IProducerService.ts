import {type ProducerProcessSchema}  from "../schema/ProducerProcessSchema";
import {type ProducerSnapshotSchema} from "../schema/ProducerSnapshotSchema";

export interface IProducerService {
    /**
     * Compute input time of the given producer process
     */
    timeOf(process: ProducerProcessSchema.Type): number;

    /**
     * Number of cycles passed by the given process
     */
    cycles(process: ProducerProcessSchema.Type): number;

    process(process: ProducerProcessSchema.Type): ProducerSnapshotSchema.Type;
}
