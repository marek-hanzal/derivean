import {withService}            from "@use-pico/server";
import {type KingdomRepository} from "../repository/KingdomRepository";

export const withKingdomRepository = withService<KingdomRepository.Type>("@derivean/kingdom/KingdomRepository");
