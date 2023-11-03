import {type IProducerProcess}  from "./IProducerProcess";
import {type IProducerSnapshot} from "./IProducerSnapshot";

export interface IProducerService {
    /**
     * Number of cycles passed by the given process
     */
    cycles(process: IProducerProcess): number;

    process(process: IProducerProcess): IProducerSnapshot;
}
