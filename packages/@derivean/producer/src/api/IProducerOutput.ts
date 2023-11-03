import {type IResource} from "@derivean/resource";

export interface IProducerOutput {
    /**
     * Resource on output
     */
    resource: IResource;
    /**
     * Amount of produced resource
     */
    amount: number;
}
