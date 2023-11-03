import {type IProducerInput}  from "./IProducerInput";
import {type IProducerOutput} from "./IProducerOutput";

export interface IProducer {
    input: IProducerInput[];
    output: IProducerOutput[];
    time: number;
}
