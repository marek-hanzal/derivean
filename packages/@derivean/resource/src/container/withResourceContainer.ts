import {type IContainer}        from "@use-pico/container";
import {ResourceRepository}     from "../repository/ResourceRepository";
import {withResourceRepository} from "./withResourceRepository";

export const withResourceContainer: IContainer.Register = container => {
    withResourceRepository.bind(container, ResourceRepository);
};
