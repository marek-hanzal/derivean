import {withService}            from "@use-pico/container";
import {type KingdomRepository} from "../repository/KingdomRepository";

export const withKingdomRepository = withService<KingdomRepository.Type>("@derivean/kingdom/KingdomRepository");
