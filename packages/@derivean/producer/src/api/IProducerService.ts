import {type ProducerProcessSchema}  from "../schema/ProducerProcessSchema";
import {type ProducerSnapshotSchema} from "../schema/ProducerSnapshotSchema";

export interface IProducerService {
    /**
     * Number of cycles passed by the given process
     */
    cycles(process: ProducerProcessSchema.Type): number;

    process(process: ProducerProcessSchema.Type): ProducerSnapshotSchema.Type;
}
