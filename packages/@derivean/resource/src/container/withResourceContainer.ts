import {type IContainer}        from "@use-pico2/container";
import {ResourceRepository}     from "../repository/ResourceRepository";
import {withResourceRepository} from "./withResourceRepository";

export const withResourceContainer: IContainer.Register = container => {
    withResourceRepository.bind(container, ResourceRepository);
};
